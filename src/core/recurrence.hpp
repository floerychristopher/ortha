#pragma once

#include "core/date.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ortha {

enum class Frequency { Daily, Weekly, Monthly, Yearly };

// What the next due date is calculated from.
enum class Anchor { Due, Completion };

struct RecurrenceRule {
    Frequency freq = Frequency::Daily;
    int interval = 1;                           // every N days/weeks/months/years, >= 1
    std::vector<std::chrono::weekday> weekdays{}; // weekly only; empty = same weekday as base
    Anchor anchor = Anchor::Due;

    bool operator==(const RecurrenceRule&) const = default;
};

// "FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE,FR;FROM=DUE" (BYDAY only when weekdays are set).
std::string serialize(const RecurrenceRule& rule);

// Inverse of serialize(). FREQ is required, INTERVAL defaults to 1, FROM to DUE.
// Returns nullopt for anything malformed. Weekdays come back sorted Monday..Sunday.
std::optional<RecurrenceRule> parse_recurrence(std::string_view text);

// The first due date after `base` according to the rule. Days that do not exist in the
// target month (31st, Feb 29th) fall back to the last day of that month.
Date next_due(const RecurrenceRule& rule, Date base);

} // namespace ortha
