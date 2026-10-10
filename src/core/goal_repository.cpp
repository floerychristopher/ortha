#include "core/goal_repository.hpp"

#include "core/schema.hpp"
#include "core/uuid.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace ortha {

namespace {

constexpr const char* kSelectGoal =
    "SELECT id, title, weekdays, start_date, created_at, updated_at, symbol, routine_id, "
    "position FROM goals WHERE deleted = 0";

std::int64_t to_mask(const Weekdays& days) {
    std::int64_t mask = 0;
    for (std::size_t i = 0; i < days.size(); ++i)
        if (days[i])
            mask |= std::int64_t{1} << i;
    return mask;
}

Weekdays from_mask(std::int64_t mask) {
    Weekdays days{};
    for (std::size_t i = 0; i < days.size(); ++i)
        days[i] = ((mask >> i) & 1) != 0;
    return days;
}

const char* status_text(GoalStatus status) {
    switch (status) {
    case GoalStatus::Done:
        return "done";
    case GoalStatus::Missed:
        return "missed";
    case GoalStatus::Skipped:
        return "skipped";
    default:
        return "open";
    }
}

GoalStatus status_from_text(const std::string& text) {
    if (text == "done")
        return GoalStatus::Done;
    if (text == "missed")
        return GoalStatus::Missed;
    if (text == "skipped")
        return GoalStatus::Skipped;
    return GoalStatus::Open;
}

Goal read_goal(const Statement& stmt) {
    Goal g;
    g.id = stmt.column_text(0);
    g.title = stmt.column_text(1);
    g.weekdays = from_mask(stmt.column_int(2));
    g.start = from_iso(stmt.column_text(3)).value_or(Date{});
    g.created_at = stmt.column_text(4);
    g.updated_at = stmt.column_text(5);
    g.symbol = stmt.column_text(6);
    g.routine_id = stmt.column_is_null(7) ? "" : stmt.column_text(7);
    g.position = static_cast<int>(stmt.column_int(8));
    return g;
}

// Binds the routine id, or NULL for a goal without routine.
Statement& bind_routine(Statement& stmt, int index, const std::string& routine_id) {
    return routine_id.empty() ? stmt.bind_null(index) : stmt.bind(index, routine_id);
}

} // namespace

GoalRepository::GoalRepository(Database& db) : db_(db) { migrate(db_); }

int GoalRepository::next_position(const std::string& routine_id) {
    auto stmt = db_.prepare(
        "SELECT COALESCE(MAX(position) + 1, 0) FROM goals WHERE deleted = 0 AND routine_id IS ?");
    bind_routine(stmt, 1, routine_id);
    stmt.step();
    return static_cast<int>(stmt.column_int(0));
}

void GoalRepository::insert(Goal& goal) {
    if (goal.id.empty())
        goal.id = make_uuid();
    goal.created_at = goal.updated_at = now_utc_iso();
    goal.position = next_position(goal.routine_id);
    auto stmt = db_.prepare("INSERT INTO goals (id, title, weekdays, start_date, created_at, "
                            "updated_at, symbol, routine_id, position) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    stmt.bind(1, goal.id)
        .bind(2, goal.title)
        .bind(3, to_mask(goal.weekdays))
        .bind(4, to_iso(goal.start))
        .bind(5, goal.created_at)
        .bind(6, goal.updated_at)
        .bind(7, goal.symbol);
    bind_routine(stmt, 8, goal.routine_id).bind(9, std::int64_t{goal.position}).step();
}

void GoalRepository::update(Goal& goal) {
    const auto stored = find(goal.id);
    if (stored && stored->routine_id != goal.routine_id)
        goal.position = next_position(goal.routine_id);
    goal.updated_at = now_utc_iso();
    auto stmt = db_.prepare("UPDATE goals SET title = ?, symbol = ?, weekdays = ?, updated_at = ?, "
                            "routine_id = ?, position = ? WHERE id = ?");
    stmt.bind(1, goal.title)
        .bind(2, goal.symbol)
        .bind(3, to_mask(goal.weekdays))
        .bind(4, goal.updated_at);
    bind_routine(stmt, 5, goal.routine_id).bind(6, std::int64_t{goal.position}).bind(7, goal.id).step();
}

void GoalRepository::remove(const std::string& id) {
    db_.prepare("UPDATE goals SET deleted = 1, updated_at = ? WHERE id = ?")
        .bind(1, now_utc_iso())
        .bind(2, id)
        .step();
}

std::optional<Goal> GoalRepository::find(const std::string& id) {
    auto stmt = db_.prepare(std::string(kSelectGoal) + " AND id = ?");
    stmt.bind(1, id);
    if (!stmt.step())
        return std::nullopt;
    return read_goal(stmt);
}

std::vector<Goal> GoalRepository::in_routine(const std::string& routine_id) {
    auto stmt = db_.prepare(std::string(kSelectGoal) +
                            " AND routine_id = ? ORDER BY position, created_at, rowid");
    stmt.bind(1, routine_id);
    std::vector<Goal> goals;
    while (stmt.step())
        goals.push_back(read_goal(stmt));
    return goals;
}

void GoalRepository::move(const std::string& id, int delta) {
    const auto goal = find(id);
    if (!goal)
        return;
    auto habits = in_routine(goal->routine_id);
    const auto it = std::find_if(habits.begin(), habits.end(),
                                 [&id](const Goal& g) { return g.id == id; });
    if (it == habits.end())
        return; // e.g. a goal without routine
    const auto from = it - habits.begin();
    const auto to = from + delta;
    if (to < 0 || to >= static_cast<std::ptrdiff_t>(habits.size()))
        return;
    std::swap(habits[static_cast<std::size_t>(from)], habits[static_cast<std::size_t>(to)]);
    // Renumber the whole routine, so gaps (e.g. from a habit that moved away) disappear.
    db_.transaction([&] {
        for (std::size_t i = 0; i < habits.size(); ++i) {
            db_.prepare("UPDATE goals SET position = ?, updated_at = ? WHERE id = ?")
                .bind(1, static_cast<std::int64_t>(i))
                .bind(2, now_utc_iso())
                .bind(3, habits[i].id)
                .step();
        }
    });
}

GoalStatus GoalRepository::status(const std::string& id, Date day) {
    auto stmt = db_.prepare("SELECT status FROM goal_entries WHERE goal_id = ? AND day = ?");
    stmt.bind(1, id).bind(2, to_iso(day));
    return stmt.step() ? status_from_text(stmt.column_text(0)) : GoalStatus::Open;
}

void GoalRepository::set_status(const std::string& id, Date day, GoalStatus status) {
    db_.prepare("INSERT INTO goal_entries (goal_id, day, status, updated_at) VALUES (?, ?, ?, ?) "
                "ON CONFLICT(goal_id, day) DO UPDATE SET status = excluded.status, "
                "updated_at = excluded.updated_at")
        .bind(1, id)
        .bind(2, to_iso(day))
        .bind(3, status_text(status))
        .bind(4, now_utc_iso())
        .step();
}

std::map<Date, GoalStatus> GoalRepository::statuses(const std::string& id, Date from, Date to) {
    auto stmt = db_.prepare("SELECT day, status FROM goal_entries "
                            "WHERE goal_id = ? AND day >= ? AND day <= ?");
    stmt.bind(1, id).bind(2, to_iso(from)).bind(3, to_iso(to));
    std::map<Date, GoalStatus> entries;
    while (stmt.step()) {
        if (auto day = from_iso(stmt.column_text(0)))
            entries[*day] = status_from_text(stmt.column_text(1));
    }
    return entries;
}

} // namespace ortha
