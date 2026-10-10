#include "core/routine_repository.hpp"

#include "core/schema.hpp"
#include "core/uuid.hpp"

#include <cstdint>
#include <map>

namespace ortha {

namespace {

constexpr const char* kSelectRoutine =
    "SELECT id, name, start_minute, end_minute, identity, created_at, updated_at FROM routines "
    "WHERE deleted = 0";

Routine read_routine(const Statement& stmt) {
    Routine r;
    r.id = stmt.column_text(0);
    r.name = stmt.column_text(1);
    r.start = static_cast<int>(stmt.column_int(2));
    if (!stmt.column_is_null(3))
        r.end = static_cast<int>(stmt.column_int(3));
    r.identity = stmt.column_text(4);
    r.created_at = stmt.column_text(5);
    r.updated_at = stmt.column_text(6);
    return r;
}

Statement& bind_end(Statement& stmt, int index, const std::optional<int>& end) {
    return end ? stmt.bind(index, std::int64_t{*end}) : stmt.bind_null(index);
}

} // namespace

RoutineRepository::RoutineRepository(Database& db) : db_(db) { migrate(db_); }

void RoutineRepository::insert(Routine& routine) {
    if (routine.id.empty())
        routine.id = make_uuid();
    routine.created_at = routine.updated_at = now_utc_iso();
    auto stmt = db_.prepare("INSERT INTO routines (id, name, start_minute, end_minute, identity, "
                            "created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?)");
    stmt.bind(1, routine.id).bind(2, routine.name).bind(3, std::int64_t{routine.start});
    bind_end(stmt, 4, routine.end)
        .bind(5, routine.identity)
        .bind(6, routine.created_at)
        .bind(7, routine.updated_at)
        .step();
}

void RoutineRepository::update(Routine& routine) {
    routine.updated_at = now_utc_iso();
    auto stmt = db_.prepare("UPDATE routines SET name = ?, start_minute = ?, end_minute = ?, "
                            "identity = ?, updated_at = ? WHERE id = ?");
    stmt.bind(1, routine.name).bind(2, std::int64_t{routine.start});
    bind_end(stmt, 3, routine.end)
        .bind(4, routine.identity)
        .bind(5, routine.updated_at)
        .bind(6, routine.id)
        .step();
}

void RoutineRepository::remove(const std::string& id) {
    const auto now = now_utc_iso();
    db_.transaction([&] {
        db_.prepare("UPDATE routines SET deleted = 1, updated_at = ? WHERE id = ?")
            .bind(1, now)
            .bind(2, id)
            .step();
        db_.prepare("UPDATE goals SET deleted = 1, updated_at = ? "
                    "WHERE routine_id = ? AND deleted = 0") // keep earlier deletions as they are
            .bind(1, now)
            .bind(2, id)
            .step();
    });
}

std::optional<Routine> RoutineRepository::find(const std::string& id) {
    auto stmt = db_.prepare(std::string(kSelectRoutine) + " AND id = ?");
    stmt.bind(1, id);
    if (!stmt.step())
        return std::nullopt;
    return read_routine(stmt);
}

std::vector<Routine> RoutineRepository::all() {
    auto stmt = db_.prepare(std::string(kSelectRoutine) + " ORDER BY start_minute, created_at, rowid");
    std::vector<Routine> routines;
    while (stmt.step())
        routines.push_back(read_routine(stmt));
    sort_routines(routines); // within the routine day: 0:30 after 23:00
    return routines;
}

std::vector<RoutineSummary> RoutineRepository::summaries(GoalRepository& goals, Date today,
                                                         bool scheduled_only) {
    std::vector<RoutineSummary> result;
    for (auto& routine : all()) {
        const auto habits = goals.in_routine(routine.id);
        std::map<std::string, std::map<Date, GoalStatus>> entries;
        for (const auto& habit : habits)
            entries[habit.id] = goals.statuses(habit.id, habit.start, today);
        const HabitLookup lookup = [&entries](const Goal& habit, Date day) {
            return status_on(entries.at(habit.id), day);
        };

        RoutineSummary summary;
        for (const auto& habit : habits)
            if (!scheduled_only || is_scheduled(habit, today))
                summary.habits.push_back(summarize(habit, today, [&](Date day) {
                    return lookup(habit, day);
                }));
        if (scheduled_only && summary.habits.empty())
            continue;
        summary.today = routine_day(habits, today, lookup);
        summary.streak = routine_streak(habits, today, lookup);
        summary.routine = std::move(routine);
        result.push_back(std::move(summary));
    }
    return result;
}

} // namespace ortha
