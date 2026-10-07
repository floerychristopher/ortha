#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>

#include <functional>
#include <string>
#include <vector>

namespace ortha {

// A vertical menu for popups (actions, confirmations); Enter calls `on_enter`.
// The highlighted entry is always the selected one, so resetting `*selected` before
// opening the popup also resets the highlight (and Enter acts on what is highlighted).
// Up (or k) on the first entry jumps to the last one, Down (or j) on the last one to the first.
inline ftxui::Component popup_menu(std::vector<std::string>* entries, int* selected,
                                   std::function<void()> on_enter) {
    auto option = ftxui::MenuOption::Vertical();
    option.entries = entries;
    option.selected = selected;
    option.focused_entry = selected;
    option.on_enter = std::move(on_enter);
    return ftxui::CatchEvent(ftxui::Menu(option), [entries, selected](const ftxui::Event& event) {
        const int last = static_cast<int>(entries->size()) - 1;
        if (last < 0)
            return false;
        const bool up = event == ftxui::Event::ArrowUp || event == ftxui::Event::Character('k');
        const bool down =
            event == ftxui::Event::ArrowDown || event == ftxui::Event::Character('j');
        if (up && *selected == 0) {
            *selected = last;
            return true;
        }
        if (down && *selected == last) {
            *selected = 0;
            return true;
        }
        return false;
    });
}

} // namespace ortha
