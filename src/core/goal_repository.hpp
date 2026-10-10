#pragma once

#include "core/goal.hpp"
#include "core/sqlite.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ortha {

// Stores goals and their per-day status in the SQLite database.
class GoalRepository {
  public:
    explicit GoalRepository(Database& db); // migrates the schema if needed

    // Assigns id (if empty), created_at, updated_at and the position at the end of its
    // routine. `goal.start` must be set.
    void insert(Goal& goal);
    // Saves title, symbol, weekdays and routine; bumps updated_at. A goal that moves to
    // another routine goes to the end of that routine.
    void update(Goal& goal);
    // Soft delete: the goal disappears everywhere, its history stays in the database.
    void remove(const std::string& id);

    std::optional<Goal> find(const std::string& id);
    // The habits of a routine, in their order.
    std::vector<Goal> in_routine(const std::string& routine_id);
    // Swaps the goal with its neighbour in the routine: -1 = up, +1 = down. No-op at the edge.
    void move(const std::string& id, int delta);

    GoalStatus status(const std::string& id, Date day);
    void set_status(const std::string& id, Date day, GoalStatus status);
    // Stored entries with from <= day <= to (days without an entry are Open).
    std::map<Date, GoalStatus> statuses(const std::string& id, Date from, Date to);

  private:
    int next_position(const std::string& routine_id); // the end of the routine

    Database& db_;
};

} // namespace ortha
