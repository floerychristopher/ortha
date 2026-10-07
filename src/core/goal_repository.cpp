#include "core/goal_repository.hpp"

#include "core/schema.hpp"
#include "core/uuid.hpp"

namespace ortha {

namespace {

constexpr const char* kSelectGoal =
    "SELECT id, title, weekdays, start_date, created_at, updated_at FROM goals WHERE deleted = 0";

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
    default:
        return "open";
    }
}

GoalStatus status_from_text(const std::string& text) {
    if (text == "done")
        return GoalStatus::Done;
    if (text == "missed")
        return GoalStatus::Missed;
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
    return g;
}

} // namespace

GoalRepository::GoalRepository(Database& db) : db_(db) { migrate(db_); }

void GoalRepository::insert(Goal& goal) {
    if (goal.id.empty())
        goal.id = make_uuid();
    goal.created_at = goal.updated_at = now_utc_iso();
    db_.prepare("INSERT INTO goals (id, title, weekdays, start_date, created_at, updated_at) "
                "VALUES (?, ?, ?, ?, ?, ?)")
        .bind(1, goal.id)
        .bind(2, goal.title)
        .bind(3, to_mask(goal.weekdays))
        .bind(4, to_iso(goal.start))
        .bind(5, goal.created_at)
        .bind(6, goal.updated_at)
        .step();
}

void GoalRepository::update(Goal& goal) {
    goal.updated_at = now_utc_iso();
    db_.prepare("UPDATE goals SET title = ?, weekdays = ?, updated_at = ? WHERE id = ?")
        .bind(1, goal.title)
        .bind(2, to_mask(goal.weekdays))
        .bind(3, goal.updated_at)
        .bind(4, goal.id)
        .step();
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

std::vector<Goal> GoalRepository::all() {
    auto stmt = db_.prepare(std::string(kSelectGoal) + " ORDER BY created_at, rowid");
    std::vector<Goal> goals;
    while (stmt.step())
        goals.push_back(read_goal(stmt));
    return goals;
}

std::vector<Goal> GoalRepository::scheduled_on(Date day) {
    auto goals = all();
    std::erase_if(goals, [day](const Goal& g) { return !is_scheduled(g, day); });
    return goals;
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

GoalSummary GoalRepository::summary(const Goal& goal, Date today) {
    const auto entries = statuses(goal.id, goal.start, today);
    const StatusLookup lookup = [&entries](Date day) {
        const auto it = entries.find(day);
        return it == entries.end() ? GoalStatus::Open : it->second;
    };
    GoalSummary s;
    s.goal = goal;
    s.today = lookup(today);
    s.streak = streak(goal, today, lookup);
    s.last_week = last_week(goal, today, lookup);
    return s;
}

} // namespace ortha
