#pragma once

#include "core/goal_repository.hpp"
#include "tui/task_view.hpp"

#include <functional>
#include <string>
#include <vector>

namespace ortha {

struct HomeActions {
    std::function<void()> on_new{};
    std::function<void()> on_all{};
    std::function<void()> on_goals{};
    std::function<void()> on_quit{};
};

struct HomeOptions {
    std::function<std::vector<Task>()> load_tasks;
    std::function<int()> count_open;
    std::function<std::vector<GoalSummary>()> load_goals = [] {
        return std::vector<GoalSummary>{};
    };
    std::function<void(const std::string& goal_id)> on_cycle_goal{};
    Date today;
    HomeActions actions;
    TaskListCallbacks task_callbacks;
};

// The start screen: today's goals (if any), the next few tasks ("Today") and the menu
// ("New task", "All Tasks (N)", "Goals", "Quit"). The menu has the focus initially.
ftxui::Component make_home(HomeOptions options);

} // namespace ortha
