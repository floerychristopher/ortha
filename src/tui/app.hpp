#pragma once

#include "core/bible_verses.hpp"
#include "core/goal_repository.hpp"
#include "core/task_repository.hpp"
#include "core/task_service.hpp"
#include "tui/home_view.hpp"
#include "tui/task_list_view.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <functional>
#include <optional>
#include <string>

namespace ortha {

// Owns the terminal screen and switches between the views (menu, lists, form).
class App {
  public:
    App(TaskRepository& repo, TaskService& service, GoalRepository& goals);
    // Runs until the user quits. Throws if an operation failed (after restoring the terminal).
    void run();

  private:
    // Replaces the visible view. Deferred until the current event is fully handled.
    void navigate(ftxui::Component next);
    void show(ftxui::Component next);
    // The start screen ("Today" + menu). Every other view returns here.
    void show_home();
    ftxui::Component home();
    // Everything the start screen shows except the verse.
    HomeOptions home_options();
    // Opens the task form: empty id = new task, otherwise edit. `back` runs after save/cancel.
    void show_form(std::optional<std::string> task_id, std::function<void()> back);
    void show_all();
    // Task actions (edit, toggle, delete) shared by the views; `back` re-opens the
    // calling view after editing.
    TaskListCallbacks task_callbacks(std::function<void()> back);
    // Goals overview, goal form (empty id = new goal), a goal's history and the month grid.
    void show_goals();
    void show_goal_form(std::optional<std::string> goal_id);
    void show_goal_history(const std::string& goal_id);
    void show_goal_month();

    TaskRepository& repo_;
    TaskService& service_;
    GoalRepository& goals_;
    std::optional<BibleVerse> verse_; // picked once per start (if one fits), shown on Today
    ftxui::ScreenInteractive screen_ = ftxui::ScreenInteractive::Fullscreen();
    ftxui::Component root_ = ftxui::Container::Vertical({});
    std::string error_; // set when an operation failed; reported after the UI is closed
};

} // namespace ortha
