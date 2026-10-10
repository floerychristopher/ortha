#include "tui/app.hpp"

#include "tui/error_guard.hpp"
#include "tui/goal_form.hpp"
#include "tui/goal_history_view.hpp"
#include "tui/goal_month_view.hpp"
#include "tui/home_view.hpp"
#include "tui/routine_form.hpp"
#include "tui/routines_view.hpp"
#include "tui/task_form.hpp"
#include "tui/task_list_view.hpp"

#include <ftxui/screen/terminal.hpp>

#include <random>
#include <stdexcept>

namespace ortha {

App::App(TaskRepository& repo, TaskService& service, GoalRepository& goals,
         RoutineRepository& routines)
    : repo_(repo), service_(service), goals_(goals), routines_(routines) {}

void App::run() {
    // One verse per start, chosen among those that fit completely on this terminal.
    const auto size = ftxui::Terminal::Size();
    std::mt19937 rng{std::random_device{}()};
    verse_ = pick_fitting_verse(home_options(), size.dimx, size.dimy, bible_verses(), rng);
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
    auto options = home_options();
    options.verse = verse_;
    return make_home(std::move(options));
}

std::shared_ptr<std::map<std::string, bool>> App::expanded_today() {
    const auto day = habit_today();
    if (day != expanded_day_) { // a new routine day starts with the automatic open/close
        expanded_day_ = day;
        expanded_ = std::make_shared<std::map<std::string, bool>>();
    }
    return expanded_;
}

HomeOptions App::home_options() {
    auto callbacks = task_callbacks([this] { show_home(); });
    callbacks.on_back = {}; // Esc on the start screen does nothing
    return {
        .load_tasks = [this] { return repo_.upcoming(4); },
        .count_open = [this] { return repo_.count_open(); },
        .load_routines = [this] { return routines_.summaries(goals_, habit_today(), true); },
        .now_minutes = [] { return ortha::now_minutes(); },
        .on_cycle_habit =
            [this](const std::string& id) {
                const auto day = habit_today();
                goals_.set_status(id, day, next_status(goals_.status(id, day)));
            },
        .expanded = [this] { return expanded_today(); },
        .today = today(),
        .verse = std::nullopt, // chosen once in run()
        .actions =
            {
                .on_new = [this] { show_form(std::nullopt, [this] { show_home(); }); },
                .on_all = [this] { show_all(); },
                .on_routines = [this] { show_routines(); },
                .on_quit = [this] { screen_.Exit(); },
            },
        .task_callbacks = std::move(callbacks),
    };
}

void App::show_routines() {
    navigate(make_routines_view(
        [this] { return routines_.summaries(goals_, habit_today(), false); },
        {
            .on_back = [this] { show_home(); },
            .on_new_routine = [this] { show_routine_form(std::nullopt); },
            .on_month = [this] { show_goal_month(); },
            .on_add_habit = [this](const std::string& id) { show_goal_form(std::nullopt, id); },
            .on_edit_routine = [this](const std::string& id) { show_routine_form(id); },
            .on_delete_routine = [this](const std::string& id) { routines_.remove(id); },
            .on_history = [this](const std::string& id) { show_goal_history(id); },
            .on_edit_habit = [this](const std::string& id) { show_goal_form(id); },
            .on_delete_habit = [this](const std::string& id) { goals_.remove(id); },
            .on_move = [this](const std::string& id, int delta) { goals_.move(id, delta); },
        }));
}

void App::show_routine_form(std::optional<std::string> routine_id) {
    Routine initial;
    if (routine_id) {
        auto found = routines_.find(*routine_id);
        if (!found)
            return show_routines();
        initial = std::move(*found);
    }
    const bool is_new = initial.id.empty();
    navigate(make_routine_form(
        is_new ? "New routine" : "Edit routine", std::move(initial),
        [this, is_new](Routine routine) {
            if (is_new)
                routines_.insert(routine);
            else
                routines_.update(routine);
            show_routines();
        },
        [this] { show_routines(); }));
}

void App::show_goal_form(std::optional<std::string> goal_id, std::string routine_id) {
    Goal initial;
    if (goal_id) {
        auto found = goals_.find(*goal_id);
        if (!found)
            return show_routines();
        initial = std::move(*found);
    } else {
        initial.routine_id = std::move(routine_id);
    }
    const bool is_new = initial.id.empty();
    navigate(make_goal_form(
        is_new ? "New habit" : "Edit habit", std::move(initial), routines_.all(),
        [this, is_new](Goal goal) {
            if (is_new) {
                goal.start = habit_today();
                goals_.insert(goal);
            } else {
                goals_.update(goal);
            }
            show_routines();
        },
        [this] { show_routines(); }));
}

void App::show_goal_history(const std::string& goal_id) {
    auto goal = goals_.find(goal_id);
    if (!goal)
        return show_routines();
    navigate(make_goal_history(
        goal->title,
        [this, goal = *goal] {
            const auto day = habit_today();
            const auto entries = goals_.statuses(goal.id, goal.start, day);
            std::vector<HistoryDay> days;
            for (const auto& d : scheduled_days(goal, day)) {
                days.push_back({d, status_on(entries, d)});
            }
            return days;
        },
        [this, id = goal_id](Date day) {
            goals_.set_status(id, day, next_status(goals_.status(id, day)));
        },
        [this] { show_routines(); }));
}

void App::show_goal_month() {
    navigate(make_goal_month(
        [this](std::chrono::year_month month) {
            const auto days = days_of_month(month);
            std::vector<MonthColumn> columns;
            for (const auto& routine : routines_.all()) { // in the order of the routines
                for (auto& goal : goals_.in_routine(routine.id)) {
                    auto entries = goals_.statuses(goal.id, days.front(), days.back());
                    columns.push_back({std::move(goal), std::move(entries)});
                }
            }
            return columns;
        },
        habit_today(),
        [this](const std::string& id, Date day) {
            goals_.set_status(id, day, next_status(goals_.status(id, day)));
        },
        [this] { show_routines(); }));
}

void App::show_all() {
    navigate(make_task_list(
        "All tasks", [this](bool include_done) { return repo_.all(include_done); }, today(),
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
