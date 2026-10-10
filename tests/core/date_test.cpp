#include "core/date.hpp"

#include <catch2/catch_test_macros.hpp>

#include <regex>

using namespace ortha;
using namespace std::chrono;

TEST_CASE("parse_date accepts DD.MM.YYYY") {
    CHECK(parse_date("07.10.2026") == Date{2026y, October, 7d});
    CHECK(parse_date("7.1.2026") == Date{2026y, January, 7d});
    CHECK(parse_date("  29.02.2028 ") == Date{2028y, February, 29d});
}

TEST_CASE("parse_date rejects invalid input") {
    for (auto s : {"29.02.2027", "31.04.2026", "32.13.2026", "1.1.26", "07.10.2026x", "07-10-2026",
                   "", "..2026", "007.10.2026"}) {
        INFO("input: " << s);
        CHECK_FALSE(parse_date(s).has_value());
    }
}

TEST_CASE("format and iso round trip") {
    Date d{2026y, March, 5d};
    CHECK(format_date(d) == "05.03.2026");
    CHECK(to_iso(d) == "2026-03-05");
    CHECK(from_iso("2026-03-05") == d);
    CHECK_FALSE(from_iso("garbage").has_value());
}

TEST_CASE("clock helpers") {
    CHECK(today().ok());
    CHECK(std::regex_match(now_utc_iso(),
                           std::regex(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z)")));
}

TEST_CASE("weekday_name") {
    CHECK(weekday_name(2026y / October / 5d) == "Mo");
    CHECK(weekday_name(2026y / October / 7d) == "We");
    CHECK(weekday_name(2026y / October / 11d) == "Su");
    CHECK(std::string(kWeekdayNames[0]) == "Mo");
    CHECK(std::string(kWeekdayNames[6]) == "Su");
}

TEST_CASE("due_label says today and tomorrow, otherwise the date with its weekday") {
    const Date today = 2026y / October / 7d;
    CHECK(due_label(today, today) == "today");
    CHECK(due_label(2026y / October / 8d, today) == "tomorrow");
    CHECK(due_label(2026y / October / 9d, today) == "09.10.2026 (Fr)");
    CHECK(due_label(2026y / October / 12d, today) == "12.10.2026 (Mo)");
    CHECK(due_label(2026y / October / 6d, today) == "06.10.2026 (Tu)"); // overdue keeps the date
    CHECK(due_label(2026y / November / 1d, 2026y / October / 31d) == "tomorrow");
}

TEST_CASE("now_minutes is a time of day") {
    const int now = now_minutes();
    CHECK(now >= 0);
    CHECK(now < 24 * 60);
}

TEST_CASE("previous_day crosses month and year") {
    CHECK(previous_day(2026y / October / 7d) == Date{2026y, October, 6d});
    CHECK(previous_day(2026y / March / 1d) == Date{2026y, February, 28d});
    CHECK(previous_day(2027y / January / 1d) == Date{2026y, December, 31d});
}

TEST_CASE("routine days start at 4:00") {
    CHECK(kDayStart == 4 * 60);
    CHECK(routine_minutes(4 * 60) == 0);
    CHECK(routine_minutes(23 * 60 + 30) == 19 * 60 + 30);
    CHECK(routine_minutes(30) == 20 * 60 + 30); // 0:30 belongs to the end of the day
    CHECK(routine_minutes(3 * 60 + 59) == 24 * 60 - 1);
    CHECK(habit_day(2026y / October / 11d, 2 * 60) == Date{2026y, October, 10d}); // 2:00
    CHECK(habit_day(2026y / October / 11d, 4 * 60) == Date{2026y, October, 11d}); // 4:00
}
