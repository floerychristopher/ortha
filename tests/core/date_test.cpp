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
