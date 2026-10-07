#include "tui/home_view.hpp"

#include <ftxui/component/component_options.hpp>

#include <format>
#include <memory>
#include <string>

namespace ortha {

using namespace ftxui;

Component make_home(std::function<std::vector<Task>()> load_today, std::function<int()> count_open,
                    Date today, HomeActions actions, TaskListCallbacks task_callbacks) {
    struct Menu {
        std::vector<std::string> entries{"New task", "", "Quit"};
        int selected = 0;
    };
    auto menu_state = std::make_shared<Menu>();
    auto update_count = [menu_state, count_open] {
        menu_state->entries[1] = std::format("All Tasks ({})", count_open());
    };
    update_count();

    auto view = Make<TaskView>(TaskViewOptions{
        .heading = "Today",
        .load = std::move(load_today),
        .today = today,
        .callbacks = std::move(task_callbacks),
        .empty_text = "Nothing to do.",
        .fill_height = false,
        .on_changed = update_count,
    });

    auto option = MenuOption::Vertical();
    option.entries = &menu_state->entries;
    option.selected = &menu_state->selected;
    option.on_enter = [menu_state, actions] {
        const std::function<void()>* targets[] = {&actions.on_new, &actions.on_all,
                                                  &actions.on_quit};
        if (const auto& fn = *targets[menu_state->selected])
            fn();
    };
    view->set_footer(ftxui::Menu(option));
    view->focus(TaskView::Area::Footer);
    return view;
}

} // namespace ortha
