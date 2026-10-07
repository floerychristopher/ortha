#pragma once

#include "core/task.hpp"

#include <ftxui/component/component.hpp>

#include <functional>
#include <string>

namespace ortha {

// Form for creating/editing a task. `on_save` receives the validated task;
// Esc or "Cancel" calls `on_cancel`. Enter in a single-line field saves.
ftxui::Component make_task_form(std::string heading, Task initial,
                                std::function<void(Task)> on_save,
                                std::function<void()> on_cancel);

} // namespace ortha
