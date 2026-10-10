#pragma once

#include "core/routine.hpp"

#include <ftxui/component/component.hpp>

#include <functional>
#include <string>
#include <variant>

namespace ortha {

// What the routine form edits; times as typed ("8:00").
struct RoutineFormState {
    std::string name;
    std::string start;
    std::string end; // empty = no end
    std::string identity;
};

// A new routine (without id) starts with empty times.
RoutineFormState routine_form_state_from(const Routine& routine);

// Validates the form and applies it to `base` (which keeps id and timestamps).
// Returns the updated routine or a user-facing error message.
std::variant<Routine, std::string> apply_routine_form(const RoutineFormState& state, Routine base);

// Form for creating/editing a routine. Enter in a field saves, Esc cancels.
ftxui::Component make_routine_form(std::string heading, Routine initial,
                                   std::function<void(Routine)> on_save,
                                   std::function<void()> on_cancel);

} // namespace ortha
