#pragma once

#include "core/goal_repository.hpp"

#include <ftxui/component/component.hpp>

#include <functional>
#include <string>
#include <vector>

namespace ortha {

struct GoalsCallbacks {
    std::function<void()> on_back{}; // Esc / Back
    std::function<void()> on_new{};
    std::function<void()> on_month{}; // the month grid
    std::function<void(const std::string& id)> on_history{};
    std::function<void(const std::string& id)> on_edit{};
    std::function<void(const std::string& id)> on_delete{};
};

// All goals (symbol + title) with their days, the last seven days and the current streak.
// Enter on a goal opens History / Edit / Delete (with confirmation).
ftxui::Component make_goals_view(std::function<std::vector<GoalSummary>()> load,
                                 GoalsCallbacks callbacks);

} // namespace ortha
