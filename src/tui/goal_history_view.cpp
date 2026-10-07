#include "tui/goal_history_view.hpp"

#include "tui/goal_widgets.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <memory>

namespace ortha {

using namespace ftxui;

namespace {

Element day_row(const HistoryDay& entry, bool focused, bool selected) {
    auto row = hbox({
        text(weekday_name(entry.day) + " " + format_date(entry.day) +
             "  "),
        goal_status_box(entry.status),
    });
    if (focused)
        row |= inverted;
    if (selected)
        row |= focus; // keep the selected row visible when scrolling
    return row;
}

} // namespace

Component make_goal_history(std::string title, std::function<std::vector<HistoryDay>()> load,
                            std::function<void(Date)> on_cycle, std::function<void()> on_back) {
    struct State {
        std::vector<HistoryDay> days;
        int selected = 0;
        Component rows;
    };
    auto state = std::make_shared<State>();
    state->rows = Container::Vertical({}, &state->selected);

    auto reload = [state, load] {
        state->days = load();
        state->rows->DetachAllChildren();
        for (std::size_t i = 0; i < state->days.size(); ++i) {
            MenuEntryOption option;
            option.label = format_date(state->days[i].day);
            // Raw pointer: the rows live inside `state` (a shared_ptr would form a cycle).
            option.transform = [raw = state.get(), i](const EntryState& entry) {
                return day_row(raw->days[i], entry.focused, static_cast<int>(i) == raw->selected);
            };
            state->rows->Add(MenuEntry(option));
        }
        state->selected =
            std::clamp(state->selected, 0, std::max(0, static_cast<int>(state->days.size()) - 1));
    };
    reload();

    auto view = Renderer(state->rows, [state, title] {
        auto body = state->days.empty() ? text("No days yet.") | dim
                                        : state->rows->Render() | vscroll_indicator | yframe | flex;
        return window(text(" History: " + title + " ") | bold, body);
    });
    return CatchEvent(view, [state, reload, on_cycle, on_back](const Event& event) {
        if (event == Event::Escape) {
            if (on_back)
                on_back();
            return true;
        }
        if (event == Event::Return && !state->days.empty()) {
            if (on_cycle)
                on_cycle(state->days[static_cast<std::size_t>(state->selected)].day);
            reload();
            return true;
        }
        return false;
    });
}

} // namespace ortha
