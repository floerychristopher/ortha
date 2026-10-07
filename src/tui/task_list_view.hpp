#pragma once

#include "tui/task_view.hpp"

namespace ortha {

// The "All tasks" list: a toolbar (Back, Show done) above the task rows.
// `load` receives whether done tasks should be included.
ftxui::Component make_task_list(std::string heading,
                                std::function<std::vector<Task>(bool include_done)> load,
                                Date today, TaskListCallbacks callbacks);

} // namespace ortha
