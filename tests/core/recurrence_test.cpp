#include "core/recurrence.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace std::chrono;

namespace {
RecurrenceRule rule(Frequency f, int n = 1, std::vector<weekday> days = {}) {
    return {f, n, std::move(days), Anchor::Due};
}
} // namespace

// 07.10.2026 is a Wednesday.

TEST_CASE("daily, monthly, yearly") {
    CHECK(next_due(rule(Frequency::Daily), 2026y / December / 31d) == 2027y / January / 1d);
    CHECK(next_due(rule(Frequency::Daily, 3), 2026y / October / 7d) == 2026y / October / 10d);
    CHECK(next_due(rule(Frequency::Monthly), 2026y / January / 31d) == 2026y / February / 28d);
    CHECK(next_due(rule(Frequency::Monthly), 2028y / January / 31d) == 2028y / February / 29d);
    CHECK(next_due(rule(Frequency::Monthly, 2), 2026y / November / 15d) == 2027y / January / 15d);
    CHECK(next_due(rule(Frequency::Yearly), 2028y / February / 29d) == 2029y / February / 28d);
    CHECK(next_due(rule(Frequency::Yearly, 4), 2028y / February / 29d) == 2032y / February / 29d);
}

TEST_CASE("weekly") {
    CHECK(next_due(rule(Frequency::Weekly), 2026y / October / 7d) == 2026y / October / 14d);
    CHECK(next_due(rule(Frequency::Weekly, 2), 2026y / October / 7d) == 2026y / October / 21d);
    auto mwf = std::vector<weekday>{Monday, Wednesday, Friday};
    CHECK(next_due(rule(Frequency::Weekly, 1, mwf), 2026y / October / 7d) == 2026y / October / 9d);
    CHECK(next_due(rule(Frequency::Weekly, 1, mwf), 2026y / October / 9d) == 2026y / October / 12d);
    CHECK(next_due(rule(Frequency::Weekly, 2, mwf), 2026y / October / 9d) == 2026y / October / 19d);
    CHECK(next_due(rule(Frequency::Weekly, 1, {Friday}), 2026y / October / 6d) ==
          2026y / October / 9d);
    CHECK(next_due(rule(Frequency::Weekly, 1, {Sunday}), 2026y / October / 11d) ==
          2026y / October / 18d);
}

TEST_CASE("serialize and parse") {
    RecurrenceRule r{Frequency::Weekly, 2, {Monday, Wednesday, Friday}, Anchor::Completion};
    CHECK(serialize(r) == "FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE,FR;FROM=COMPLETION");
    CHECK(parse_recurrence(serialize(r)) == r);
    CHECK(serialize(rule(Frequency::Monthly)) == "FREQ=MONTHLY;INTERVAL=1;FROM=DUE");
    CHECK(parse_recurrence("FREQ=DAILY") == rule(Frequency::Daily));
    CHECK(parse_recurrence("FREQ=WEEKLY;BYDAY=FR,MO") == rule(Frequency::Weekly, 1, {Monday, Friday}));
    for (auto bad : {"", "garbage", "FREQ=HOURLY", "FREQ=DAILY;INTERVAL=0", "FREQ=DAILY;INTERVAL=x",
                     "FREQ=DAILY;BYDAY=MO", "FREQ=WEEKLY;BYDAY=XX", "FREQ=DAILY;FROM=NOW",
                     "INTERVAL=2"}) {
        INFO(bad);
        CHECK_FALSE(parse_recurrence(bad).has_value());
    }
}
