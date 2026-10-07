#pragma once

#include "core/date.hpp"

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ortha {

// How a goal went on one day. Days without an entry are Open.
enum class GoalStatus { Open, Done, Missed };

using Weekdays = std::array<bool, 7>; // Monday..Sunday

// A daily goal ("habit"), e.g. "Go to the gym" on Monday, Wednesday and Friday.
struct Goal {
    std::string id; // UUID, assigned on insert
    std::string title;
    std::string symbol; // short column label (emoji or letters); empty = none
    Weekdays weekdays{true, true, true, true, true, true, true};
    Date start; // local date the goal was created; earlier days never count
    std::string created_at;
    std::string updated_at;
};

// Looks up the status of the goal on a given day.
using StatusLookup = std::function<GoalStatus(Date)>;

// 0 = Monday .. 6 = Sunday
int weekday_index(Date day);

// True if the goal applies on `day`: its weekday is chosen and the goal already existed.
bool is_scheduled(const Goal& goal, Date day);

// The status after pressing Enter: Open -> Done -> Missed -> Open.
GoalStatus next_status(GoalStatus status);

// Number of consecutive scheduled days with Done, counted back from today.
// An open (not yet rated) today does not break the streak; a missed or unrated
// past day does.
int streak(const Goal& goal, Date today, const StatusLookup& status_of);

// The last seven days ending with today, oldest first; nullopt = not scheduled.
std::array<std::optional<GoalStatus>, 7> last_week(const Goal& goal, Date today,
                                                   const StatusLookup& status_of);

// All scheduled days from the start date to today, newest first.
std::vector<Date> scheduled_days(const Goal& goal, Date today);

// "daily" if every day is chosen, otherwise e.g. "Mo We Fr".
std::string weekdays_label(const Weekdays& days);

// All days of the month, first to last.
std::vector<Date> days_of_month(std::chrono::year_month month);

// What one cell of the month grid shows. Only Open, Done and Missed can be changed.
enum class CellState { Future, NotScheduled, Open, Done, Missed };

// NotScheduled if the goal does not apply on `day` (also in the future), Future if a
// planned day is still ahead, otherwise the given status.
CellState month_cell(const Goal& goal, Date day, Date today, GoalStatus status);

} // namespace ortha
