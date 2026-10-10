#pragma once

#include "core/goal_repository.hpp"
#include "core/routine.hpp"
#include "core/sqlite.hpp"

#include <optional>
#include <string>
#include <vector>

namespace ortha {

// Everything a view needs to show one routine.
struct RoutineSummary {
    Routine routine;
    std::vector<GoalSummary> habits; // in their order
    RoutineDay today;                // over all habits of the routine
    int streak = 0;
};

// Stores routines in the SQLite database. Their habits are goals (see GoalRepository).
class RoutineRepository {
  public:
    explicit RoutineRepository(Database& db); // migrates the schema if needed

    // Assigns id (if empty), created_at and updated_at.
    void insert(Routine& routine);
    // Saves name, times and identity; bumps updated_at.
    void update(Routine& routine);
    // Soft delete of the routine and its habits; their history stays in the database.
    void remove(const std::string& id);

    std::optional<Routine> find(const std::string& id);
    std::vector<Routine> all(); // by start within the routine day (see sort_routines)

    // All routines with their habits and today's progress and streaks. With scheduled_only,
    // only the habits planned on `today` are listed, and routines without such habits are
    // left out (the Today view). Progress and streaks always cover all habits.
    std::vector<RoutineSummary> summaries(GoalRepository& goals, Date today, bool scheduled_only);

  private:
    Database& db_;
};

} // namespace ortha
