#pragma once

#include "core/task.hpp"

#include <array>
#include <string>
#include <variant>
#include <vector>

namespace ortha {

// What the task form edits: plain strings/indices, exactly as typed by the user.
struct TaskFormState {
    std::string title;
    std::string due; // "DD.MM.YYYY" or empty
    std::string tags; // comma separated
    std::string notes;
    int priority = 0; // index into {"none", "low", "medium", "high"}
    std::vector<Subtask> subtasks{};
    int repeat = 0;               // index into {"none", "daily", "weekly", "monthly", "yearly"}
    std::string interval = "1";   // "every N ..."
    std::array<bool, 7> weekdays{}; // Monday..Sunday, weekly only
    int anchor = 0;               // index into {"due date", "completion"}
};

TaskFormState form_state_from(const Task& task);

// Validates the form and applies it to `base` (which keeps id, timestamps, done state, ...).
// Returns the updated task or a user-facing error message.
std::variant<Task, std::string> apply_form(const TaskFormState& state, Task base);

} // namespace ortha
