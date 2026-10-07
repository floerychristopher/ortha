#include "tui/app.hpp"

#include "tui/error_guard.hpp"
#include "tui/goal_form.hpp"
#include "tui/goal_history_view.hpp"
#include "tui/goal_month_view.hpp"
#include "tui/goals_view.hpp"
#include "tui/home_view.hpp"
#include "tui/task_form.hpp"
#include "tui/task_list_view.hpp"

#include <random>
#include <stdexcept>

namespace ortha {

namespace {

const BibleVerse& random_verse() {
    const auto verses = bible_verses();
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> pick{0, verses.size() - 1};
    return verses[pick(rng)];
}

} // namespace

App::App(TaskRepository& repo, TaskService& service, GoalRepository& goals)
    : repo_(repo), service_(service), goals_(goals), verse_(random_verse()) {}

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
    return make_home({
        .load_tasks = [this] { return repo_.upcoming(3); },
        .count_open = [this] { return repo_.count_open(); },
        .load_goals =
            [this] {
                const auto day = today();
                std::vector<GoalSummary> summaries;
                for (const auto& goal : goals_.scheduled_on(day))
                    summaries.push_back(goals_.summary(goal, day));
                return summaries;
            },
        .on_cycle_goal =
            [this](const std::string& id) {
                const auto day = today();
                goals_.set_status(id, day, next_status(goals_.status(id, day)));
            },
        .today = today(),
        .verse = verse_,
        .actions =
            {
                .on_new = [this] { show_form(std::nullopt, [this] { show_home(); }); },
                .on_all = [this] { show_all(); },
                .on_goals = [this] { show_goals(); },
                .on_quit = [this] { screen_.Exit(); },
            },
        .task_callbacks = std::move(callbacks),
    });
}

void App::show_goals() {
    navigate(make_goals_view(
        [this] {
            const auto day = today();
            std::vector<GoalSummary> summaries;
            for (const auto& goal : goals_.all())
                summaries.push_back(goals_.summary(goal, day));
            return summaries;
        },
        {
            .on_back = [this] { show_home(); },
            .on_new = [this] { show_goal_form(std::nullopt); },
            .on_month = [this] { show_goal_month(); },
            .on_history = [this](const std::string& id) { show_goal_history(id); },
            .on_edit = [this](const std::string& id) { show_goal_form(id); },
            .on_delete = [this](const std::string& id) { goals_.remove(id); },
        }));
}

void App::show_goal_form(std::optional<std::string> goal_id) {
    Goal initial;
    if (goal_id) {
        auto found = goals_.find(*goal_id);
        if (!found)
            return show_goals();
        initial = std::move(*found);
    }
    const bool is_new = initial.id.empty();
    navigate(make_goal_form(
        is_new ? "New goal" : "Edit goal", std::move(initial),
        [this, is_new](Goal goal) {
            if (is_new) {
                goal.start = today();
                goals_.insert(goal);
            } else {
                goals_.update(goal);
            }
            show_goals();
        },
        [this] { show_goals(); }));
}

void App::show_goal_history(const std::string& goal_id) {
    auto goal = goals_.find(goal_id);
    if (!goal)
        return show_goals();
    navigate(make_goal_history(
        goal->title,
        [this, goal = *goal] {
            const auto day = today();
            const auto entries = goals_.statuses(goal.id, goal.start, day);
            std::vector<HistoryDay> days;
            for (const auto& d : scheduled_days(goal, day)) {
                const auto it = entries.find(d);
                days.push_back({d, it == entries.end() ? GoalStatus::Open : it->second});
            }
            return days;
        },
        [this, id = goal_id](Date day) {
            goals_.set_status(id, day, next_status(goals_.status(id, day)));
        },
        [this] { show_goals(); }));
}

void App::show_goal_month() {
    navigate(make_goal_month(
        [this](std::chrono::year_month month) {
            const auto days = days_of_month(month);
            std::vector<MonthColumn> columns;
            for (auto& goal : goals_.all()) {
                auto entries = goals_.statuses(goal.id, days.front(), days.back());
                columns.push_back({std::move(goal), std::move(entries)});
            }
            return columns;
        },
        today(),
        [this](const std::string& id, Date day) {
            goals_.set_status(id, day, next_status(goals_.status(id, day)));
        },
        [this] { show_goals(); }));
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
