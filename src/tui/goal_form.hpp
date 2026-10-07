#pragma once

#include "core/goal.hpp"

#include <ftxui/component/component.hpp>

#include <functional>
#include <string>
#include <variant>

namespace ortha {

// What the goal form edits.
struct GoalFormState {
    std::string title;
    Weekdays weekdays{true, true, true, true, true, true, true};
};

GoalFormState goal_form_state_from(const Goal& goal);

// Validates the form and applies it to `base` (which keeps id, start date, timestamps).
// Returns the updated goal or a user-facing error message.
std::variant<Goal, std::string> apply_goal_form(const GoalFormState& state, Goal base);

// Form for creating/editing a goal. Enter in the title saves, Esc cancels.
ftxui::Component make_goal_form(std::string heading, Goal initial,
                                std::function<void(Goal)> on_save,
                                std::function<void()> on_cancel);

} // namespace ortha
