#include "core/goal.hpp"

#include <catch2/catch_test_macros.hpp>

#include <map>

using namespace ortha;
using namespace std::chrono;

// 07.10.2026 is a Wednesday.

namespace {

Goal daily(Date start) {
    Goal g;
    g.title = "x";
    g.start = start;
    return g;
}

Goal mwf(Date start) {
    Goal g = daily(start);
    g.weekdays = {true, false, true, false, true, false, false};
    return g;
}

StatusLookup lookup(std::map<Date, GoalStatus> entries) {
    return [entries = std::move(entries)](Date day) {
        auto it = entries.find(day);
        return it == entries.end() ? GoalStatus::Open : it->second;
    };
}

constexpr auto Done = GoalStatus::Done;
constexpr auto Missed = GoalStatus::Missed;
constexpr auto Open = GoalStatus::Open;

} // namespace

TEST_CASE("is_scheduled respects weekdays and start date") {
    auto g = mwf(2026y / October / 5d);
    CHECK(is_scheduled(g, 2026y / October / 7d));       // Wed
    CHECK_FALSE(is_scheduled(g, 2026y / October / 6d)); // Tue
    CHECK_FALSE(is_scheduled(g, 2026y / October / 2d)); // Fri, before start
    CHECK(weekday_index(2026y / October / 5d) == 0);    // Monday
    CHECK(weekday_index(2026y / October / 11d) == 6);   // Sunday
}

TEST_CASE("next_status cycles") {
    CHECK(next_status(Open) == Done);
    CHECK(next_status(Done) == Missed);
    CHECK(next_status(Missed) == Open);
}

TEST_CASE("streak counts consecutive done days") {
    auto g = daily(2026y / October / 1d);
    const Date today = 2026y / October / 7d;
    std::map<Date, GoalStatus> past{{2026y / October / 4d, Done},
                                    {2026y / October / 5d, Done},
                                    {2026y / October / 6d, Done}};
    CHECK(streak(g, today, lookup(past)) == 3); // today still open

    auto with_today = past;
    with_today[today] = Done;
    CHECK(streak(g, today, lookup(with_today)) == 4);
    with_today[today] = Missed;
    CHECK(streak(g, today, lookup(with_today)) == 0);

    CHECK(streak(g, today,
                 lookup({{2026y / October / 6d, Done}, {2026y / October / 4d, Done}})) == 1);
    CHECK(streak(g, today,
                 lookup({{2026y / October / 6d, Done},
                         {2026y / October / 5d, Missed},
                         {2026y / October / 4d, Done}})) == 1);
}

TEST_CASE("streak skips unscheduled days") {
    auto g = mwf(2026y / September / 28d);
    CHECK(streak(g, 2026y / October / 7d,
                 lookup({{2026y / October / 5d, Done},
                         {2026y / October / 2d, Done},
                         {2026y / September / 30d, Done},
                         {2026y / September / 28d, Done}})) == 4);
}

TEST_CASE("streak ignores entries on days no longer scheduled") {
    auto g = mwf(2026y / October / 1d);
    CHECK(streak(g, 2026y / October / 7d,
                 lookup({{2026y / October / 7d, Done},
                         {2026y / October / 6d, Missed}, // Tuesday: not scheduled any more
                         {2026y / October / 5d, Done}})) == 2);
}

TEST_CASE("streak stops at the start date") {
    auto g = daily(2026y / October / 6d);
    CHECK(streak(g, 2026y / October / 7d,
                 lookup({{2026y / October / 7d, Done},
                         {2026y / October / 6d, Done},
                         {2026y / October / 5d, Done}})) == 2);
}

TEST_CASE("last_week shows the last seven days, oldest first") {
    auto g = mwf(2026y / October / 1d);
    auto week = last_week(g, 2026y / October / 7d,
                          lookup({{2026y / October / 2d, Done}, {2026y / October / 7d, Missed}}));
    std::array<std::optional<GoalStatus>, 7> expected{std::nullopt, Done,         std::nullopt,
                                                      std::nullopt, Open,         std::nullopt,
                                                      Missed};
    CHECK(week == expected);

    auto fresh = last_week(daily(2026y / October / 6d), 2026y / October / 7d, lookup({}));
    std::array<std::optional<GoalStatus>, 7> expected_fresh{
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, Open, Open};
    CHECK(fresh == expected_fresh);
}

TEST_CASE("scheduled_days lists start..today, newest first") {
    CHECK(scheduled_days(daily(2026y / October / 6d), 2026y / October / 7d) ==
          std::vector<Date>{2026y / October / 7d, 2026y / October / 6d});
    CHECK(scheduled_days(mwf(2026y / October / 1d), 2026y / October / 7d) ==
          std::vector<Date>{2026y / October / 7d, 2026y / October / 5d, 2026y / October / 2d});
}

TEST_CASE("weekdays_label") {
    CHECK(weekdays_label({true, true, true, true, true, true, true}) == "daily");
    CHECK(weekdays_label({true, false, true, false, true, false, false}) == "Mo We Fr");
    CHECK(weekdays_label({false, false, false, false, false, true, true}) == "Sa Su");
}
