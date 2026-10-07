#pragma once

#include "core/goal.hpp"

#include <ftxui/component/component.hpp>

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ortha {

// One goal of the month grid with its stored entries in that month.
struct MonthColumn {
    Goal goal;
    std::map<Date, GoalStatus> entries; // days without an entry are Open
};

// Habit-tracker grid for one month: goals as columns, days as rows.
// Enter cycles the focused cell (only open, done or missed cells), PgUp/PgDn change the
// month, Esc goes back.
ftxui::Component
make_goal_month(std::function<std::vector<MonthColumn>(std::chrono::year_month)> load, Date today,
                std::function<void(const std::string& goal_id, Date day)> on_cycle,
                std::function<void()> on_back);

} // namespace ortha
