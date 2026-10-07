#pragma once

#include "core/goal.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/string.hpp>

#include <string>

namespace ortha {

// The widest symbol a goal may have, in terminal columns (2 emoji or 4 letters).
inline constexpr int kMaxSymbolWidth = 4;

// The goal's symbol, or the first kMaxSymbolWidth columns of its title.
inline std::string goal_symbol(const Goal& goal) {
    if (!goal.symbol.empty())
        return goal.symbol;
    std::string symbol;
    for (const auto& glyph : ftxui::Utf8ToGlyphs(goal.title)) {
        if (ftxui::string_width(symbol + glyph) > kMaxSymbolWidth)
            break;
        symbol += glyph;
    }
    return symbol;
}

// "[ ]", "[✓]" (green) or "[✗]" (red).
inline ftxui::Element goal_status_box(GoalStatus status) {
    using namespace ftxui;
    switch (status) {
    case GoalStatus::Done:
        return text("[✓]") | color(Color::Green);
    case GoalStatus::Missed:
        return text("[✗]") | color(Color::Red);
    default:
        return text("[ ]");
    }
}

} // namespace ortha
