#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>

#include <functional>
#include <string>
#include <vector>

namespace ortha {

// A vertical menu for popups (actions, confirmations); Enter calls `on_enter`.
inline ftxui::Component popup_menu(std::vector<std::string>* entries, int* selected,
                                   std::function<void()> on_enter) {
    auto option = ftxui::MenuOption::Vertical();
    option.entries = entries;
    option.selected = selected;
    option.on_enter = std::move(on_enter);
    return ftxui::Menu(option);
}

} // namespace ortha
