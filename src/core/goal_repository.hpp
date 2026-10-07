#pragma once

#include "core/goal.hpp"
#include "core/sqlite.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ortha {

// Everything a view needs to show one goal.
struct GoalSummary {
    Goal goal;
    GoalStatus today = GoalStatus::Open;
    int streak = 0;
    std::array<std::optional<GoalStatus>, 7> last_week{}; // oldest first, nullopt = not scheduled
};

// Stores goals and their per-day status in the SQLite database.
class GoalRepository {
  public:
    explicit GoalRepository(Database& db); // migrates the schema if needed

    // Assigns id (if empty), created_at and updated_at. `goal.start` must be set.
    void insert(Goal& goal);
    // Saves title and weekdays; bumps updated_at.
    void update(Goal& goal);
    // Soft delete: the goal disappears everywhere, its history stays in the database.
    void remove(const std::string& id);

    std::optional<Goal> find(const std::string& id);
    std::vector<Goal> all(); // in creation order
    std::vector<Goal> scheduled_on(Date day);

    GoalStatus status(const std::string& id, Date day);
    void set_status(const std::string& id, Date day, GoalStatus status);
    // Stored entries with from <= day <= to (days without an entry are Open).
    std::map<Date, GoalStatus> statuses(const std::string& id, Date from, Date to);

    GoalSummary summary(const Goal& goal, Date today);

  private:
    Database& db_;
};

} // namespace ortha
