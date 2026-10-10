#pragma once

#include "core/bible_verses.hpp"
#include "core/routine_repository.hpp"
#include "tui/task_view.hpp"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <vector>

namespace ortha {

struct HomeActions {
    std::function<void()> on_new{};
    std::function<void()> on_all{};
    std::function<void()> on_routines{};
    std::function<void()> on_quit{};
};

struct HomeOptions {
    std::function<std::vector<Task>()> load_tasks;
    std::function<int()> count_open;
    // Today's routines (only the habits planned today), sorted by start time.
    std::function<std::vector<RoutineSummary>()> load_routines = [] {
        return std::vector<RoutineSummary>{};
    };
    // Local time of day in minutes; asked on every reload, so the current routine moves on
    // while Today stays open.
    std::function<int()> now_minutes = [] { return 0; };
    std::function<void(const std::string& goal_id)> on_cycle_habit{};
    // Routines opened (true) or closed (false) by hand, by routine id. Asked on every reload:
    // the caller keeps the choices while Today is built again and starts afresh each day.
    std::function<std::shared_ptr<std::map<std::string, bool>>()> expanded =
        [choices = std::make_shared<std::map<std::string, bool>>()] { return choices; };
    Date today;
    std::optional<BibleVerse> verse; // shown below the menu
    HomeActions actions;
    TaskListCallbacks task_callbacks;
};

// The start screen: today's routines (if any), the next few tasks ("Today") and the menu
// ("New task", "All tasks (N)", "Routines", "Quit"), then the bible verse (if any).
// The menu has the focus initially. Enter on a habit cycles its status, Enter on a
// routine heading opens or closes it. The routine on now is cyan (▶), begun routines with
// open habits are yellow.
ftxui::Component make_home(HomeOptions options);

// The rows the start screen needs at `width` columns (including the verse, if any).
int home_height(const HomeOptions& options, int width);

// A random verse from `verses` that fits completely on a `width` x `height` start screen
// next to everything else in `options` (goals, tasks, menu); nullopt if none fits.
std::optional<BibleVerse> pick_fitting_verse(const HomeOptions& options, int width, int height,
                                             std::span<const BibleVerse> verses,
                                             std::mt19937& rng);

} // namespace ortha
