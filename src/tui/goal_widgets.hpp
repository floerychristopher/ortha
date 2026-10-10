#pragma once

#include "core/goal.hpp"
#include "core/text.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/string.hpp>

#include <format>
#include <optional>
#include <string>

namespace ortha {

// The widest symbol a goal may have, in terminal columns (2 emoji or 4 letters).
inline constexpr int kMaxSymbolWidth = 4;

// The goal's symbol, or the first kMaxSymbolWidth columns of its title (without
// trailing spaces).
inline std::string goal_symbol(const Goal& goal) {
    if (!goal.symbol.empty())
        return goal.symbol;
    std::string symbol;
    for (const auto& glyph : ftxui::Utf8ToGlyphs(goal.title)) {
        if (ftxui::string_width(symbol + glyph) > kMaxSymbolWidth)
            break;
        symbol += glyph;
    }
    return trim(symbol);
}

// "[ ]", "[✓]" (green), "[✗]" (red) or "[·]" (dim: rest day, not planned today).
inline ftxui::Element goal_status_box(GoalStatus status) {
    using namespace ftxui;
    switch (status) {
    case GoalStatus::Done:
        return text("[✓]") | color(Color::Green);
    case GoalStatus::Missed:
        return text("[✗]") | color(Color::Red);
    case GoalStatus::Skipped:
        return text("[·]") | dim;
    case GoalStatus::Open:
        break;
    }
    return text("[ ]");
}

// One character for a day: "✓" (green), "✗" (red), "○" open, "·" (dim) not planned —
// also for a rest day (Skipped) and for nullopt.
inline ftxui::Element status_mark(std::optional<GoalStatus> status) {
    using namespace ftxui;
    switch (status.value_or(GoalStatus::Skipped)) {
    case GoalStatus::Done:
        return text("✓") | color(Color::Green);
    case GoalStatus::Missed:
        return text("✗") | color(Color::Red);
    case GoalStatus::Open:
        return text("○");
    case GoalStatus::Skipped:
        break;
    }
    return text("·") | dim;
}

// "🔥 N", right-aligned in `width` columns; empty without a streak.
inline ftxui::Element streak_label(int streak, int width) {
    using namespace ftxui;
    return hbox({filler(), text(streak > 0 ? std::format("🔥 {}", streak) : "")}) |
           size(WIDTH, EQUAL, width);
}

// How a habit is listed: "🙏 Pray", or just "Pray" without a symbol.
inline std::string habit_label(const Goal& goal) {
    return goal.symbol.empty() ? goal.title : goal.symbol + " " + goal.title;
}

} // namespace ortha
