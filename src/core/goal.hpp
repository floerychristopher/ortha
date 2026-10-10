#pragma once

#include "core/date.hpp"

#include <array>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ortha {

// How a goal went on one day. Days without an entry are Open. Skipped is a rest day: the
// goal is not planned that day (shown as "·"), so it neither counts nor breaks a streak.
enum class GoalStatus { Open, Done, Missed, Skipped };

using Weekdays = std::array<bool, 7>; // Monday..Sunday

// A daily goal, e.g. "Go to the gym" on Monday, Wednesday and Friday. The UI calls it a
// "habit": every goal is one step of a routine (see core/routine.hpp).
struct Goal {
    std::string id; // UUID, assigned on insert
    std::string title;
    std::string symbol; // short column label (emoji or letters); empty = none
    Weekdays weekdays{true, true, true, true, true, true, true};
    Date start{}; // local date the goal was created; earlier days never count
    std::string routine_id; // the routine it belongs to; empty = none
    int position = 0;       // order within the routine, first = 0
    std::string created_at;
    std::string updated_at;
};

// Looks up the status of the goal on a given day.
using StatusLookup = std::function<GoalStatus(Date)>;

// The status stored for `day`; days without an entry are Open.
GoalStatus status_on(const std::map<Date, GoalStatus>& entries, Date day);

// How one day counts for a "never miss twice" chain: passed over, done, or a miss.
enum class DayRating { Pass, Done, Miss };

// Number of Done days in the chain counted back from `today` to `first`: a single Miss is
// bridged without counting, two Misses in a row end the chain, Pass days are passed over.
int chain_length(Date today, Date first, const std::function<DayRating(Date)>& rate);

// 0 = Monday .. 6 = Sunday
int weekday_index(Date day);

// True if the goal applies on `day`: its weekday is chosen and the goal already existed.
bool is_scheduled(const Goal& goal, Date day);

// The status after pressing Enter: Open -> Done -> Missed -> Skipped -> Open.
GoalStatus next_status(GoalStatus status);

// Number of Done days in the current chain, counted back from today ("never miss twice"):
// a single miss (Missed, or a past day left Open) is bridged without counting, two misses in
// a row end the chain. Unscheduled and Skipped days are passed over; an open today is not
// rated yet and passed over too.
int streak(const Goal& goal, Date today, const StatusLookup& status_of);

// True if today is still Open and the last scheduled, not skipped day before today was a
// miss (Missed or left Open).
bool missed_last_time(const Goal& goal, Date today, const StatusLookup& status_of);

// The last seven days ending with today, oldest first; nullopt = not scheduled.
std::array<std::optional<GoalStatus>, 7> last_week(const Goal& goal, Date today,
                                                   const StatusLookup& status_of);

// All scheduled days from the start date to today, newest first.
std::vector<Date> scheduled_days(const Goal& goal, Date today);

// Everything a view needs to show one goal.
struct GoalSummary {
    Goal goal;
    GoalStatus today = GoalStatus::Open;
    int streak = 0;
    std::array<std::optional<GoalStatus>, 7> last_week{}; // oldest first, nullopt = not scheduled
    bool missed_last_time = false;
};

GoalSummary summarize(const Goal& goal, Date today, const StatusLookup& status_of);

// "daily" if every day is chosen, otherwise e.g. "Mo We Fr".
std::string weekdays_label(const Weekdays& days);

// All days of the month, first to last.
std::vector<Date> days_of_month(std::chrono::year_month month);

// What one cell of the month grid shows. Future and NotScheduled cells cannot be changed.
enum class CellState { Future, NotScheduled, Open, Done, Missed, Skipped };

// NotScheduled if the goal does not apply on `day` (also in the future), Skipped for a rest
// day (also in the future), Future if a planned day is still ahead, otherwise the status.
CellState month_cell(const Goal& goal, Date day, Date today, GoalStatus status);

} // namespace ortha
