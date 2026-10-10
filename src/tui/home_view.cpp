#include "tui/home_view.hpp"

#include "tui/goal_widgets.hpp"
#include "tui/routine_lines.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include <algorithm>
#include <cstddef>
#include <format>
#include <optional>
#include <memory>
#include <string>

namespace ortha {

using namespace ftxui;

namespace {

// "2/2 ✓", "0/1", or "·" when the whole routine has a rest day.
std::string progress_label(const RoutineDay& day) {
    if (day.planned == 0)
        return "·";
    auto label = std::format("{}/{}", day.done, day.planned);
    if (day.done == day.planned)
        label += " ✓";
    return label;
}

// on_now: the routine's time window is now (cyan, ▶); needs_care: it has begun and still
// has open habits (yellow).
Element routine_heading(const RoutineSummary& r, bool expanded, bool focused, bool on_now,
                        bool needs_care) {
    auto line = hbox({
        text(on_now ? "▶" : " "),
        text(r.routine.name) | bold | flex_shrink,
        filler(),
        text("  " + time_label(r.routine)) | size(WIDTH, EQUAL, 15),
        text(progress_label(r.today)) | size(WIDTH, EQUAL, 8), // up to "10/10 ✓" and a space
        streak_label(r.streak, 7), // lines up with the habits' streaks below
    });
    if (on_now)
        line |= color(Color::Cyan);
    else if (needs_care)
        line |= color(Color::Yellow);
    if (focused)
        line |= inverted;
    if (!expanded || r.routine.identity.empty())
        return line;
    return vbox({line, text("  " + r.routine.identity) | dim | italic});
}

Element habit_row(const GoalSummary& habit, bool next, bool focused) {
    const auto& goal = habit.goal;
    auto row = hbox({
        text(next ? "> " : "  "),
        goal_status_box(habit.today),
        text(" "),
        text(habit_label(goal)) | flex_shrink,
        filler(),
        habit.missed_last_time ? text("  missed last time") | dim : streak_label(habit.streak, 7),
    });
    return focused ? row | inverted : row;
}

// Today's routines: a heading per routine, the habits of the expanded ones below it.
Component routines_section(std::function<std::vector<RoutineSummary>()> load,
                           std::function<int()> now,
                           std::function<void(const std::string&)> on_cycle_habit,
                           std::function<std::shared_ptr<std::map<std::string, bool>>()> opened) {
    struct State {
        std::vector<RoutineSummary> routines;
        std::vector<bool> expanded;
        std::vector<bool> on_now;
        std::vector<bool> needs_care;
        std::vector<RoutineLine> lines;
        int selected = 0;
        // The routine of the habit rated last stays expanded, so the cursor stays on that
        // habit and the next Enter keeps cycling it (instead of collapsing under the cursor).
        std::string keep_open;
        Component rows;
    };
    auto state = std::make_shared<State>();
    state->rows = Container::Vertical({}, &state->selected);

    auto reload = [state, load, now, opened] {
        // Keep the selection on the same heading or habit, even when lines come and go.
        const auto before =
            state->lines.empty()
                ? std::string()
                : line_key(state->routines, state->lines[static_cast<std::size_t>(state->selected)]);

        state->routines = load();
        state->expanded.clear();
        state->on_now.clear();
        state->needs_care.clear();
        std::vector<Routine> plain;
        for (const auto& r : state->routines)
            plain.push_back(r.routine);
        const int at = now();
        const auto choices = opened(); // today's choices, fresh after the day change
        const auto current = plain.empty() ? 0 : current_routine(plain, at);
        for (std::size_t i = 0; i < state->routines.size(); ++i) {
            const auto& r = state->routines[i];
            const auto by_hand = choices->find(r.routine.id);
            state->expanded.push_back(by_hand != choices->end()
                                          ? by_hand->second
                                          : is_expanded(i, current, r.today) ||
                                                r.routine.id == state->keep_open);
            state->on_now.push_back(in_window(plain, i, at));
            state->needs_care.push_back(r.today.open > 0 &&
                                        routine_minutes(r.routine.start) <= routine_minutes(at));
        }
        state->lines = routine_lines(state->routines,
                                     [&](std::size_t i) { return state->expanded[i]; });

        state->rows->DetachAllChildren();
        for (std::size_t n = 0; n < state->lines.size(); ++n) {
            MenuEntryOption option;
            // Raw pointer: the rows live inside `state` (a shared_ptr would form a cycle).
            option.transform = [raw = state.get(), n](const EntryState& entry) {
                const auto line = raw->lines[n];
                const auto& r = raw->routines[line.routine];
                if (!line.habit)
                    return routine_heading(r, raw->expanded[line.routine], entry.focused,
                                           raw->on_now[line.routine],
                                           raw->needs_care[line.routine]);
                const auto first_open = std::find_if(r.habits.begin(), r.habits.end(),
                                                     [](const GoalSummary& h) {
                                                         return h.today == GoalStatus::Open;
                                                     });
                const bool next = first_open - r.habits.begin() ==
                                  static_cast<std::ptrdiff_t>(*line.habit);
                return habit_row(r.habits[*line.habit], next, entry.focused);
            };
            state->rows->Add(MenuEntry(option));
        }
        state->selected =
            static_cast<int>(find_line(state->routines, state->lines, before).value_or(0));
    };
    reload();

    auto view = Renderer(state->rows, [state] { return state->rows->Render(); });
    return CatchEvent(view, [state, reload, on_cycle_habit, opened](const Event& event) {
        if (event != Event::Return || state->lines.empty())
            return false;
        const auto line = state->lines[static_cast<std::size_t>(state->selected)];
        const auto& r = state->routines[line.routine];
        if (line.habit) {
            state->keep_open = r.routine.id;
            if (on_cycle_habit)
                on_cycle_habit(r.habits[*line.habit].goal.id);
        } else { // open or close the routine; the choice is kept
            (*opened())[r.routine.id] = !state->expanded[line.routine];
            state->keep_open.clear();
        }
        reload();
        return true;
    });
}

// The verse text (wrapped) with its reference right-aligned below it.
Element verse_box(const BibleVerse& verse) {
    return vbox({
        paragraph(std::string(verse.text)) | italic,
        hbox({filler(), text(std::string(verse.reference)) | dim}),
    });
}

// The separator line plus the verse, as drawn below the menu.
Element verse_section(const BibleVerse& verse) { return vbox({separator(), verse_box(verse)}); }

// The number of rows `element` fills when drawn `width` columns wide (trailing blank rows
// do not count). Measured with FTXUI itself, so wrapping matches the real screen. The
// element sits in a vbox like the app's views do, so it gets its natural height instead
// of being stretched to the whole screen.
int used_rows(const Element& element, int width) {
    constexpr int kMaxRows = 200;
    auto screen = Screen::Create(Dimension::Fixed(width), Dimension::Fixed(kMaxRows));
    Render(screen, vbox({element}));
    int rows = 0;
    for (int y = 0; y < kMaxRows; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto& c = screen.PixelAt(x, y).character;
            if (!c.empty() && c != " ") {
                rows = y + 1;
                break;
            }
        }
    }
    return rows;
}

} // namespace

Component make_home(HomeOptions options) {
    struct Menu {
        std::vector<std::string> entries{"New task", "", "Routines", "Quit"};
        int selected = 0;
    };
    auto menu_state = std::make_shared<Menu>();
    auto update_count = [menu_state, count_open = options.count_open] {
        menu_state->entries[1] = std::format("All tasks ({})", count_open());
    };
    update_count();

    auto view = Make<TaskView>(TaskViewOptions{
        .heading = "Today",
        .load = std::move(options.load_tasks),
        .today = options.today,
        .callbacks = std::move(options.task_callbacks),
        .empty_text = "nothing to do.",
        .fill_height = false,
        .on_changed = update_count,
    });

    if (!options.load_routines().empty())
        view->set_header(routines_section(options.load_routines, options.now_minutes,
                                          options.on_cycle_habit, options.expanded));

    auto menu_option = MenuOption::Vertical();
    menu_option.entries = &menu_state->entries;
    menu_option.selected = &menu_state->selected;
    menu_option.on_enter = [menu_state, actions = options.actions] {
        const std::function<void()>* targets[] = {&actions.on_new, &actions.on_all,
                                                  &actions.on_routines, &actions.on_quit};
        if (const auto& fn = *targets[menu_state->selected])
            fn();
    };
    auto menu = ftxui::Menu(menu_option);
    if (options.verse) {
        menu = Renderer(menu, [menu, verse = *options.verse] {
            return vbox({menu->Render(), separator(), verse_box(verse)});
        });
    }
    view->set_footer(menu);
    view->focus(TaskView::Area::Footer);
    return view;
}

int home_height(const HomeOptions& options, int width) {
    return used_rows(make_home(options)->Render(), width);
}

std::optional<BibleVerse> pick_fitting_verse(const HomeOptions& options, int width, int height,
                                             std::span<const BibleVerse> verses,
                                             std::mt19937& rng) {
    auto without_verse = options;
    without_verse.verse.reset();
    const int free_rows = height - home_height(without_verse, width);
    const int inner_width = width - 2; // inside the window border
    std::vector<BibleVerse> fitting;
    for (const auto& verse : verses)
        if (used_rows(verse_section(verse), inner_width) <= free_rows)
            fitting.push_back(verse);
    if (fitting.empty())
        return std::nullopt;
    std::uniform_int_distribution<std::size_t> pick{0, fitting.size() - 1};
    return fitting[pick(rng)];
}

} // namespace ortha
