#pragma once

#include "core/goal.hpp"
#include "core/routine.hpp"

#include <ftxui/component/component.hpp>

#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace ortha {

// What the habit (goal) form edits.
struct GoalFormState {
    std::string title;
    Weekdays weekdays{true, true, true, true, true, true, true};
    std::string symbol;
    int routine = 0; // index into the routines the form offers
};

// The goal's routine is preselected; an unknown routine selects the first one.
GoalFormState goal_form_state_from(const Goal& goal, const std::vector<Routine>& routines);

// Validates the form and applies it to `base` (which keeps id, start date, timestamps).
// Returns the updated goal or a user-facing error message.
std::variant<Goal, std::string> apply_goal_form(const GoalFormState& state, Goal base,
                                                const std::vector<Routine>& routines);

// Form for creating/editing a habit, including the routine it belongs to.
// Enter in the title or symbol field saves, Esc cancels.
ftxui::Component make_goal_form(std::string heading, Goal initial, std::vector<Routine> routines,
                                std::function<void(Goal)> on_save,
                                std::function<void()> on_cancel);

} // namespace ortha
