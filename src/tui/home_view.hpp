#pragma once

#include "tui/task_view.hpp"

#include <functional>
#include <vector>

namespace ortha {

struct HomeActions {
    std::function<void()> on_new{};
    std::function<void()> on_all{};
    std::function<void()> on_quit{};
};

// The start screen: the next few tasks ("Today") with the menu below
// ("New task", "All Tasks (N)", "Quit"). The menu has the focus initially.
ftxui::Component make_home(std::function<std::vector<Task>()> load_today,
                           std::function<int()> count_open, Date today, HomeActions actions,
                           TaskListCallbacks task_callbacks);

} // namespace ortha
