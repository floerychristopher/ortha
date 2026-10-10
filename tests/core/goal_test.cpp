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
constexpr auto Skipped = GoalStatus::Skipped;

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
    CHECK(next_status(Missed) == Skipped);
    CHECK(next_status(Skipped) == Open);
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
    CHECK(streak(g, today, lookup(with_today)) == 3); // one miss does not break the chain

    // An unrated past day is a miss too, but a single one is bridged.
    CHECK(streak(g, today,
                 lookup({{2026y / October / 6d, Done}, {2026y / October / 4d, Done}})) == 2);
    CHECK(streak(g, today,
                 lookup({{2026y / October / 6d, Done},
                         {2026y / October / 5d, Missed},
                         {2026y / October / 4d, Done}})) == 2);
}

TEST_CASE("two misses in a row end the streak") {
    auto g = daily(2026y / October / 1d);
    const Date today = 2026y / October / 7d;
    CHECK(streak(g, today,
                 lookup({{2026y / October / 6d, Missed},
                         {2026y / October / 5d, Missed},
                         {2026y / October / 4d, Done},
                         {2026y / October / 3d, Done}})) == 0);
    CHECK(streak(g, today,
                 lookup({{2026y / October / 6d, Done},
                         {2026y / October / 5d, Missed},
                         {2026y / October / 4d, Missed},
                         {2026y / October / 3d, Done}})) == 1);
    CHECK(streak(g, today, lookup({{today, Missed}, {2026y / October / 6d, Missed},
                                   {2026y / October / 5d, Done}})) == 0);
}

TEST_CASE("skipped days neither count nor break") {
    auto g = daily(2026y / October / 1d);
    const Date today = 2026y / October / 7d;
    // skipped is passed over, so the miss on the 5th is followed by the done on the 4th
    CHECK(streak(g, today,
                 lookup({{2026y / October / 6d, Skipped},
                         {2026y / October / 5d, Missed},
                         {2026y / October / 4d, Done},
                         {2026y / October / 3d, Missed},
                         {2026y / October / 2d, Missed}})) == 1);
    CHECK(streak(g, today, lookup({{2026y / October / 6d, Skipped}, {2026y / October / 5d, Done},
                                   {2026y / October / 4d, Missed},
                                   {2026y / October / 3d, Missed}})) == 1);
    CHECK(streak(g, today, lookup({{today, Skipped}, {2026y / October / 6d, Done},
                                   {2026y / October / 5d, Missed},
                                   {2026y / October / 4d, Missed}})) == 1);
}

TEST_CASE("missed_last_time looks at the last planned day") {
    const Date today = 2026y / October / 7d; // Wednesday
    auto g = daily(2026y / October / 1d);
    CHECK(missed_last_time(g, today, lookup({{2026y / October / 6d, Missed}})));
    CHECK(missed_last_time(g, today, lookup({}))); // yesterday left unrated
    CHECK_FALSE(missed_last_time(g, today, lookup({{2026y / October / 6d, Done}})));
    CHECK_FALSE(missed_last_time(
        g, today, lookup({{today, Done}, {2026y / October / 6d, Missed}}))); // today rated
    CHECK(missed_last_time(
        g, today, lookup({{2026y / October / 6d, Skipped}, {2026y / October / 5d, Missed}})));

    auto gym = mwf(2026y / September / 28d);
    CHECK(missed_last_time(gym, today, lookup({}))); // Monday unrated, Tuesday not planned
    CHECK_FALSE(missed_last_time(gym, today, lookup({{2026y / October / 5d, Done}})));
    CHECK_FALSE(missed_last_time(daily(today), today, lookup({}))); // nothing before today
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

TEST_CASE("days_of_month lists every day of the month") {
    auto oct = days_of_month(2026y / October);
    CHECK(oct.size() == 31);
    CHECK(oct.front() == Date{2026y, October, 1d});
    CHECK(oct.back() == Date{2026y, October, 31d});
    CHECK(days_of_month(2026y / November).size() == 30);
    CHECK(days_of_month(2026y / February).size() == 28);
    CHECK(days_of_month(2028y / February).size() == 29);
}

TEST_CASE("month_cell") {
    const Date today = 2026y / October / 7d;
    auto g = mwf(2026y / October / 5d);
    CHECK(month_cell(g, 2026y / October / 9d, today, GoalStatus::Done) == CellState::Future);
    CHECK(month_cell(g, 2026y / October / 8d, today, GoalStatus::Open) ==
          CellState::NotScheduled); // a future Thursday is never rated either
    CHECK(month_cell(g, 2026y / October / 2d, today, GoalStatus::Open) ==
          CellState::NotScheduled); // before the start date
    CHECK(month_cell(g, 2026y / October / 6d, today, GoalStatus::Open) ==
          CellState::NotScheduled); // Tuesday
    CHECK(month_cell(g, today, today, GoalStatus::Open) == CellState::Open);
    CHECK(month_cell(g, 2026y / October / 5d, today, GoalStatus::Done) == CellState::Done);
    CHECK(month_cell(g, today, today, GoalStatus::Missed) == CellState::Missed);
}

TEST_CASE("month_cell shows skipped days") {
    const Date today = 2026y / October / 7d;
    auto g = mwf(2026y / October / 5d);
    CHECK(month_cell(g, 2026y / October / 5d, today, Skipped) == CellState::Skipped);
    CHECK(month_cell(g, 2026y / October / 9d, today, Skipped) == CellState::Skipped); // future
    CHECK(month_cell(g, 2026y / October / 6d, today, Skipped) ==
          CellState::NotScheduled); // Tuesday: not planned anyway
}

TEST_CASE("status_on: days without an entry are open") {
    const std::map<Date, GoalStatus> entries{{2026y / October / 6d, Done}};
    CHECK(status_on(entries, 2026y / October / 6d) == Done);
    CHECK(status_on(entries, 2026y / October / 7d) == Open);
}

TEST_CASE("chain_length bridges one miss, two end the chain") {
    using enum DayRating;
    std::map<Date, DayRating> days{{2026y / October / 7d, Pass}, {2026y / October / 6d, Done},
                                   {2026y / October / 5d, Miss}, {2026y / October / 4d, Done},
                                   {2026y / October / 3d, Miss}, {2026y / October / 2d, Miss},
                                   {2026y / October / 1d, Done}};
    auto rate = [&](Date d) { return days.at(d); };
    CHECK(chain_length(2026y / October / 7d, 2026y / October / 1d, rate) == 2);
    CHECK(chain_length(2026y / October / 7d, 2026y / October / 4d, rate) == 2); // stops at first
    CHECK(chain_length(2026y / October / 1d, 2026y / October / 1d, rate) == 1);
}

TEST_CASE("summarize combines today, streak, last week and the hint") {
    auto g = daily(2026y / October / 1d);
    g.id = "g";
    const Date today = 2026y / October / 7d;
    auto s = summarize(g, today, lookup({{2026y / October / 5d, Done}, {2026y / October / 6d, Missed}}));
    CHECK(s.goal.id == "g");
    CHECK(s.today == Open);
    CHECK(s.streak == 1);
    CHECK(s.last_week[5] == Missed);
    CHECK(s.missed_last_time);
}
