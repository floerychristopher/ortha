#pragma once

#include "core/goal.hpp"

#include <ftxui/dom/elements.hpp>

namespace ortha {

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
