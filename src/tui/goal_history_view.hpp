#pragma once

#include "core/goal.hpp"

#include <ftxui/component/component.hpp>

#include <functional>
#include <string>
#include <vector>

namespace ortha {

struct HistoryDay {
    Date day;
    GoalStatus status;
};

// The scheduled days of one goal (newest first, as delivered by `load`).
// Enter cycles the selected day's status (backfilling), Esc goes back.
ftxui::Component make_goal_history(std::string title, std::function<std::vector<HistoryDay>()> load,
                                   std::function<void(Date)> on_cycle,
                                   std::function<void()> on_back);

} // namespace ortha
