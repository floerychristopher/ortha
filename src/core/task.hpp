#pragma once

#include "core/date.hpp"
#include "core/recurrence.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ortha {

enum class Priority { None = 0, Low = 1, Medium = 2, High = 3 };

struct Subtask {
    std::string id;
    std::string title;
    bool done = false;
};

struct Task {
    std::string id; // UUID, assigned on insert
    std::string title;
    std::string notes;
    std::optional<Date> due;
    Priority priority = Priority::None;
    bool done = false;
    std::optional<std::string> completed_at; // UTC timestamp
    std::optional<RecurrenceRule> recurrence;
    std::vector<std::string> tags;            // normalized, sorted
    std::vector<Subtask> subtasks;            // in display order
    std::string created_at;                   // UTC timestamps, set by the repository
    std::string updated_at;
};

// "Work, home ,work" -> {"home", "work"}: trimmed, lowercase, sorted, no duplicates or blanks.
std::vector<std::string> parse_tags(std::string_view input);

// {"home", "work"} -> "home, work"
std::string join_tags(const std::vector<std::string>& tags);

} // namespace ortha
