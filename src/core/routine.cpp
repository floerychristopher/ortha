#include "core/routine.hpp"

#include "core/text.hpp"

#include <algorithm>
#include <charconv>
#include <format>

namespace ortha {

namespace ch = std::chrono;

namespace {

// Only plain digits: from_chars would also accept a leading "-".
std::optional<int> parse_number(std::string_view digits) {
    if (digits.empty() || !std::all_of(digits.begin(), digits.end(),
                                       [](char c) { return c >= '0' && c <= '9'; }))
        return std::nullopt;
    int value = 0;
    const auto* end = digits.data() + digits.size();
    auto [ptr, ec] = std::from_chars(digits.data(), end, value);
    if (ec != std::errc{} || ptr != end)
        return std::nullopt;
    return value;
}

} // namespace

std::optional<int> parse_time(std::string_view text) {
    const auto trimmed = trim(text);
    const auto colon = trimmed.find(':');
    if (colon == std::string::npos || colon == 0 || colon > 2 || trimmed.size() - colon != 3)
        return std::nullopt;
    const auto hours = parse_number(std::string_view(trimmed).substr(0, colon));
    const auto minutes = parse_number(std::string_view(trimmed).substr(colon + 1));
    if (!hours || !minutes || *hours > 23 || *minutes > 59)
        return std::nullopt;
    return *hours * 60 + *minutes;
}

std::string format_time(int minutes) { return std::format("{}:{:02}", minutes / 60, minutes % 60); }

std::string time_label(const Routine& routine) {
    auto label = format_time(routine.start);
    if (routine.end)
        label += "–" + format_time(*routine.end);
    return label;
}

void sort_routines(std::vector<Routine>& routines) {
    std::stable_sort(routines.begin(), routines.end(), [](const Routine& a, const Routine& b) {
        const auto sa = routine_minutes(a.start), sb = routine_minutes(b.start);
        return sa != sb ? sa < sb : a.created_at < b.created_at;
    });
}

std::size_t current_routine(const std::vector<Routine>& routines, int now) {
    std::size_t current = 0;
    for (std::size_t i = 0; i < routines.size(); ++i)
        if (routine_minutes(routines[i].start) <= routine_minutes(now))
            current = i;
    return current;
}

int routine_end_minutes(int clock_minutes) {
    const int end = routine_minutes(clock_minutes);
    return end == 0 ? 24 * 60 : end;
}

bool in_window(const std::vector<Routine>& routines, std::size_t index, int now) {
    const auto& r = routines[index];
    const int start = routine_minutes(r.start);
    int end = 24 * 60; // the day change
    if (r.end)
        end = routine_end_minutes(*r.end);
    else if (index + 1 < routines.size())
        end = routine_minutes(routines[index + 1].start);
    const int at = routine_minutes(now);
    return start <= at && at < end;
}

RoutineDay routine_day(const std::vector<Goal>& habits, Date day, const HabitLookup& status_of) {
    RoutineDay result;
    for (const auto& habit : habits) {
        if (!is_scheduled(habit, day))
            continue;
        switch (status_of(habit, day)) {
        case GoalStatus::Skipped:
            ++result.skipped;
            continue;
        case GoalStatus::Done:
            ++result.done;
            break;
        case GoalStatus::Missed:
            ++result.missed;
            break;
        case GoalStatus::Open:
            ++result.open;
            break;
        }
        ++result.planned;
    }
    return result;
}

int routine_streak(const std::vector<Goal>& habits, Date today, const HabitLookup& status_of) {
    if (habits.empty())
        return 0;
    const auto first = std::min_element(habits.begin(), habits.end(), [](const Goal& a,
                                                                         const Goal& b) {
                           return a.start < b.start;
                       })->start;
    return chain_length(today, first, [&](Date day) {
        const auto result = routine_day(habits, day, status_of);
        if (result.planned == 0)
            return DayRating::Pass;
        if (result.done == result.planned)
            return DayRating::Done;
        if (day == today && result.missed == 0)
            return DayRating::Pass; // today is still going
        return DayRating::Miss;
    });
}

bool is_expanded(std::size_t index, std::size_t current, const RoutineDay& today) {
    return today.open > 0 && index <= current;
}

} // namespace ortha
