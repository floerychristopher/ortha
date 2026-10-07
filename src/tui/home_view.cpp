#include "tui/home_view.hpp"

#include "tui/goal_widgets.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include <format>
#include <memory>
#include <string>

namespace ortha {

using namespace ftxui;

namespace {

Element goal_row(const GoalSummary& goal, bool focused) {
    auto row = hbox({
        goal_status_box(goal.today),
        text(" "),
        text(goal.goal.title) | flex_shrink,
        filler(),
        text(goal.streak > 0 ? std::format("  🔥 {}", goal.streak) : ""),
    });
    return focused ? row | inverted : row;
}

// "Goals" heading with one row per goal; Enter cycles the selected goal's status.
Component goals_section(std::function<std::vector<GoalSummary>()> load,
                        std::function<void(const std::string&)> on_cycle) {
    struct State {
        std::vector<GoalSummary> goals;
        int selected = 0;
        Component rows;
    };
    auto state = std::make_shared<State>();
    state->rows = Container::Vertical({}, &state->selected);

    auto reload = [state, load] {
        state->goals = load();
        state->rows->DetachAllChildren();
        for (std::size_t i = 0; i < state->goals.size(); ++i) {
            MenuEntryOption option;
            option.label = state->goals[i].goal.title;
            // Capture the raw pointer: the rows live inside `state`, a shared_ptr capture
            // here would keep `state` alive forever (reference cycle).
            option.transform = [raw = state.get(), i](const EntryState& entry) {
                return goal_row(raw->goals[i], entry.focused);
            };
            state->rows->Add(MenuEntry(option));
        }
    };
    reload();

    auto view = Renderer(state->rows, [state] {
        return vbox({text("Goals") | bold, state->rows->Render()});
    });
    return CatchEvent(view, [state, reload, on_cycle](const Event& event) {
        if (event != Event::Return || state->goals.empty())
            return false;
        if (on_cycle)
            on_cycle(state->goals[static_cast<std::size_t>(state->selected)].goal.id);
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
        std::vector<std::string> entries{"New task", "", "Goals", "Quit"};
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

    if (!options.load_goals().empty())
        view->set_header(goals_section(options.load_goals, options.on_cycle_goal));

    auto menu_option = MenuOption::Vertical();
    menu_option.entries = &menu_state->entries;
    menu_option.selected = &menu_state->selected;
    menu_option.on_enter = [menu_state, actions = options.actions] {
        const std::function<void()>* targets[] = {&actions.on_new, &actions.on_all,
                                                  &actions.on_goals, &actions.on_quit};
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
