#pragma once

#include "core/date.hpp"
#include "core/goal.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ortha {

// An ordered group of habits (goals) with a time window, e.g. "Morning routine" at 8:00.
struct Routine {
    std::string id; // UUID, assigned on insert
    std::string name;
    int start = 0;          // minutes since midnight, 8:00 = 480
    std::optional<int> end; // minutes since midnight; none = open end
    std::string identity;   // who the routine makes you, e.g. "I start the day with prayer"
    std::string created_at;
    std::string updated_at;
};

// "8:00", "08:00" or "23:59" (surrounding whitespace ignored) -> minutes since midnight.
std::optional<int> parse_time(std::string_view text);

// 480 -> "8:00", 1170 -> "19:30".
std::string format_time(int minutes);

// "8:00", or "19:30–21:30" when the routine has an end.
std::string time_label(const Routine& routine);

// Sorts by start within the routine day (so 0:30 comes after 23:00), then by creation.
void sort_routines(std::vector<Routine>& routines);

// The routine that is on at `now` (clock minutes): the last one (in the given order, sorted
// with sort_routines) that has begun this routine day; before the first one has begun, the
// first one. `routines` must not be empty.
std::size_t current_routine(const std::vector<Routine>& routines, int now);

// The end of a routine within its routine day; an end at 4:00 is the day change (1440).
int routine_end_minutes(int clock_minutes);

// Whether `now` (clock minutes) lies in the time window of routines[index]: from its start to
// its end, or without an end to the start of the next routine, or to the day change.
bool in_window(const std::vector<Routine>& routines, std::size_t index, int now);

// Looks up the status of a habit on a day.
using HabitLookup = std::function<GoalStatus(const Goal&, Date)>;

// The habits of a routine that are planned on one day. Skipped habits only count in
// `skipped`, not in `planned`.
struct RoutineDay {
    int planned = 0;
    int done = 0;
    int open = 0;
    int missed = 0;
    int skipped = 0;
};

RoutineDay routine_day(const std::vector<Goal>& habits, Date day, const HabitLookup& status_of);

// Number of days in the current chain on which every planned habit was done ("never miss
// twice", like streak() for a single habit). Days without planned habits are passed over;
// today only counts once it is complete, and is a miss as soon as a habit is missed.
int routine_streak(const std::vector<Goal>& habits, Date today, const HabitLookup& status_of);

// Whether Today shows the habits of the routine at `index`: while it still has open habits
// and it is the current routine or began earlier.
bool is_expanded(std::size_t index, std::size_t current, const RoutineDay& today);

} // namespace ortha
