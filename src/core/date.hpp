#pragma once

#include <array>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace ortha {

// A calendar day without time of day, e.g. 2026-10-07.
using Date = std::chrono::year_month_day;

// Parses "DD.MM.YYYY" (day and month may have one or two digits, surrounding
// whitespace is ignored). Returns nullopt for malformed or non-existent dates.
std::optional<Date> parse_date(std::string_view text);

// Short weekday names, Monday first.
inline constexpr std::array<const char*, 7> kWeekdayNames{"Mo", "Tu", "We", "Th",
                                                          "Fr", "Sa", "Su"};

// "We" for a Wednesday.
std::string weekday_name(Date d);

// "05.03.2026" - the format shown to the user.
std::string format_date(Date d);

// How a due date is shown in task lists: "today", "tomorrow" or e.g. "12.10.2026 (Mo)".
std::string due_label(Date due, Date today);

// "2026-03-05" - the format stored in the database.
std::string to_iso(Date d);
std::optional<Date> from_iso(std::string_view text);

// The day before `d`.
Date previous_day(Date d);

// The current date in the local time zone.
Date today();

// The current local time of day in minutes since midnight (0..1439).
int now_minutes();

// For routines and habits the day changes at 4:00, not at midnight, so a night routine
// that runs past midnight still belongs to its evening.
inline constexpr int kDayStart = 4 * 60;

// A time of day (minutes since midnight) as minutes since the day change: 4:00 -> 0,
// 23:30 -> 1170, 0:30 -> 1230 (late in the same routine day).
int routine_minutes(int clock_minutes);

// The routine day at `clock_minutes` on the calendar `date`: before 4:00 it is the day before.
Date habit_day(Date date, int clock_minutes);

// The routine day right now.
Date habit_today();

// The current time in UTC, e.g. "2026-10-07T08:15:30Z".
std::string now_utc_iso();

} // namespace ortha
