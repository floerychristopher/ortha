#include "tui/app.hpp"

#include "tui/error_guard.hpp"
#include "tui/home_view.hpp"
#include "tui/task_form.hpp"
#include "tui/task_list_view.hpp"

#include <stdexcept>

namespace ortha {

App::App(TaskRepository& repo, TaskService& service) : repo_(repo), service_(service) {}

void App::run() {
    show(home());
    screen_.Loop(guard_errors(root_, [this](const std::string& message) {
        error_ = message;
        screen_.Exit();
    }));
    if (!error_.empty())
        throw std::runtime_error(error_);
}

void App::navigate(ftxui::Component next) {
    screen_.Post([this, next] { show(next); });
}

void App::show(ftxui::Component next) {
    root_->DetachAllChildren();
    root_->Add(next);
}

void App::show_home() { navigate(home()); }

ftxui::Component App::home() {
    auto callbacks = task_callbacks([this] { show_home(); });
    callbacks.on_back = {}; // Esc on the start screen does nothing
    return make_home([this] { return repo_.upcoming(3); }, [this] { return repo_.count_open(); },
                     today(),
                     {
                         .on_new = [this] { show_form(std::nullopt, [this] { show_home(); }); },
                         .on_all = [this] { show_all(); },
                         .on_quit = [this] { screen_.Exit(); },
                     },
                     std::move(callbacks));
}

void App::show_all() {
    navigate(make_task_list(
        "All Tasks", [this](bool include_done) { return repo_.all(include_done); }, today(),
        task_callbacks([this] { show_all(); })));
}

TaskListCallbacks App::task_callbacks(std::function<void()> back) {
    return {
        .on_back = [this] { show_home(); },
        .on_edit = [this, back](const std::string& id) { show_form(id, back); },
        .on_toggle_done =
            [this](const std::string& id) {
                auto task = repo_.find(id);
                if (task && task->done)
                    service_.reopen(id);
                else
                    service_.complete(id, today());
            },
        .on_delete = [this](const std::string& id) { repo_.remove(id); },
    };
}

void App::show_form(std::optional<std::string> task_id, std::function<void()> back) {
    Task initial;
    if (task_id) {
        auto found = repo_.find(*task_id);
        if (!found)
            return back();
        initial = std::move(*found);
    }
    const bool is_new = initial.id.empty();
    navigate(make_task_form(
        is_new ? "New task" : "Edit task", std::move(initial),
        [this, is_new, back](Task task) {
            if (is_new)
                repo_.insert(task);
            else
                repo_.update(task);
            back();
        },
        back));
}

} // namespace ortha
