#include "tui/task_list_view.hpp"

#include <ftxui/component/component_options.hpp>

#include <memory>

namespace ortha {

using namespace ftxui;

Component make_task_list(std::string heading, std::function<std::vector<Task>(bool)> load,
                         Date today, TaskListCallbacks callbacks) {
    struct State {
        bool show_done = false;
        std::string label = "Show done: off";
    };
    auto state = std::make_shared<State>();

    auto view = Make<TaskView>(TaskViewOptions{
        .heading = std::move(heading),
        .load = [state, load] { return load(state->show_done); },
        .today = today,
        .callbacks = callbacks,
        .show_notes = true,
    });

    // The toolbar lives inside the view, so capturing the raw pointer is safe.
    auto* view_ptr = view.get();
    auto back = Button(
        "Back", [callbacks] { callbacks.on_back ? callbacks.on_back() : void(); },
        ButtonOption::Ascii());
    auto toggle_done = Button(
        &state->label,
        [state, view_ptr] {
            state->show_done = !state->show_done;
            state->label = state->show_done ? "Show done: on" : "Show done: off";
            view_ptr->reload();
        },
        ButtonOption::Ascii());
    view->set_header(Container::Horizontal({back, toggle_done}));
    return view;
}

} // namespace ortha
