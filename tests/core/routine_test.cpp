#include "core/routine.hpp"

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <utility>

using namespace ortha;
using namespace std::chrono;

// 07.10.2026 is a Wednesday.

namespace {

constexpr auto Done = GoalStatus::Done;
constexpr auto Missed = GoalStatus::Missed;
constexpr auto Skipped = GoalStatus::Skipped;

Goal habit(std::string id, Date start, Weekdays days = {true, true, true, true, true, true, true}) {
    Goal g;
    g.id = std::move(id);
    g.title = g.id;
    g.start = start;
    g.weekdays = days;
    return g;
}

Routine routine(std::string name, int start) {
    Routine r;
    r.name = std::move(name);
    r.start = start;
    return r;
}

using Entries = std::map<std::pair<std::string, Date>, GoalStatus>;

HabitLookup lookup(Entries entries) {
    return [entries = std::move(entries)](const Goal& g, Date day) {
        auto it = entries.find({g.id, day});
        return it == entries.end() ? GoalStatus::Open : it->second;
    };
}

constexpr Date kToday = 2026y / October / 7d;

} // namespace

TEST_CASE("parse_time reads H:MM and HH:MM") {
    CHECK(parse_time("8:00") == 480);
    CHECK(parse_time(" 08:00 ") == 480);
    CHECK(parse_time("0:00") == 0);
    CHECK(parse_time("23:59") == 1439);
    for (auto bad : {"24:00", "8:60", "8", "8:5", "abc", "", "-1:00", "8:00x"}) {
        INFO(bad);
        CHECK_FALSE(parse_time(bad));
    }
}

TEST_CASE("format_time and time_label") {
    CHECK(format_time(480) == "8:00");
    CHECK(format_time(1170) == "19:30");
    CHECK(format_time(0) == "0:00");
    auto sport = routine("Sport", 1170);
    CHECK(time_label(sport) == "19:30");
    sport.end = 1290;
    CHECK(time_label(sport) == "19:30–21:30");
}

TEST_CASE("current_routine is the last one that has begun") {
    const std::vector<Routine> day{routine("Morning", 480), routine("Focus", 600),
                                   routine("Sport", 1170), routine("Night", 1380)};
    CHECK(current_routine(day, 360) == 0);  // 6:00, nothing begun yet: the first one
    CHECK(current_routine(day, 480) == 0);  // 8:00
    CHECK(current_routine(day, 840) == 1);  // 14:00
    CHECK(current_routine(day, 1200) == 2); // 20:00
    CHECK(current_routine(day, 1430) == 3); // 23:50
    const std::vector<Routine> same{routine("A", 480), routine("B", 480)};
    CHECK(current_routine(same, 500) == 1); // same start: the later one in the list
}

TEST_CASE("routine_day counts the habits planned on a day") {
    const std::vector<Goal> habits{habit("a", 2026y / October / 1d), habit("b", 2026y / October / 1d),
                                   habit("c", 2026y / October / 1d), habit("d", 2026y / October / 1d),
                                   habit("gym", 2026y / October / 1d,
                                         {true, false, true, false, true, false, false})};
    auto day = routine_day(habits, 2026y / October / 6d, // Tuesday: no gym
                           lookup({{{"a", 2026y / October / 6d}, Done},
                                   {{"b", 2026y / October / 6d}, Missed},
                                   {{"c", 2026y / October / 6d}, Skipped}}));
    CHECK(day.planned == 3); // a, b, d (c is skipped, gym not planned)
    CHECK(day.done == 1);
    CHECK(day.missed == 1);
    CHECK(day.open == 1);
    CHECK(day.skipped == 1);
}

TEST_CASE("routine_streak counts fully done days, never miss twice") {
    const std::vector<Goal> habits{habit("a", 2026y / October / 1d), habit("b", 2026y / October / 1d)};
    auto done_both = [](Entries& e, Date d) {
        e[{"a", d}] = Done;
        e[{"b", d}] = Done;
    };
    Entries e;
    done_both(e, 2026y / October / 6d);
    e[{"a", 2026y / October / 5d}] = Done; // b left open: incomplete day = a miss, bridged
    done_both(e, 2026y / October / 4d);
    // 3rd and 2nd unrated: two misses end the chain
    CHECK(routine_streak(habits, kToday, lookup(e)) == 2); // today still open

    done_both(e, kToday);
    CHECK(routine_streak(habits, kToday, lookup(e)) == 3);

    e[{"b", kToday}] = Missed; // a miss today, 6th done: bridged
    CHECK(routine_streak(habits, kToday, lookup(e)) == 2);

    Entries rest; // a day where everything is skipped is passed over
    rest[{"a", 2026y / October / 6d}] = Skipped;
    rest[{"b", 2026y / October / 6d}] = Skipped;
    done_both(rest, 2026y / October / 5d);
    CHECK(routine_streak(habits, kToday, lookup(rest)) == 1);
}

TEST_CASE("is_expanded: current or earlier routines with open habits") {
    RoutineDay open;
    open.planned = 2;
    open.open = 1;
    RoutineDay finished;
    finished.planned = 2;
    finished.done = 2;
    CHECK(is_expanded(1, 1, open));      // current, open
    CHECK_FALSE(is_expanded(1, 1, finished)); // current, all done
    CHECK(is_expanded(0, 1, open));      // earlier, still open
    CHECK_FALSE(is_expanded(2, 1, open)); // not begun yet
}

TEST_CASE("routine_streak of a routine without habits is 0") {
    CHECK(routine_streak({}, kToday, lookup({})) == 0);
}

TEST_CASE("routines after midnight belong to the evening before") {
    const std::vector<Routine> day{routine("Morning", 480), routine("Night", 23 * 60 + 30),
                                   routine("Late", 30)}; // sorted by routine day
    CHECK(current_routine(day, 15) == 1);      // 0:15: the night routine is on
    CHECK(current_routine(day, 45) == 2);      // 0:45: "Late" (0:30) has begun
    CHECK(current_routine(day, 5 * 60) == 0);  // 5:00: a new day, before the morning
}

TEST_CASE("sort_routines orders by the routine day") {
    std::vector<Routine> rs{routine("Late", 30), routine("Night", 1380), routine("Morning", 480)};
    rs[0].created_at = rs[1].created_at = rs[2].created_at = "x";
    sort_routines(rs);
    CHECK(rs[0].name == "Morning");
    CHECK(rs[1].name == "Night");
    CHECK(rs[2].name == "Late");
}

TEST_CASE("in_window: from start to the end, the next start, or the day change") {
    auto focus = routine("Focus", 10 * 60);
    focus.end = 16 * 60;
    auto night = routine("Night", 23 * 60 + 30);
    night.end = 30; // 0:30, past midnight
    const std::vector<Routine> day{routine("Morning", 8 * 60), focus, routine("Sport", 19 * 60),
                                   night};
    CHECK(in_window(day, 0, 9 * 60));        // morning, no end: until focus begins
    CHECK_FALSE(in_window(day, 0, 10 * 60)); // focus has begun
    CHECK(in_window(day, 1, 15 * 60));
    CHECK_FALSE(in_window(day, 1, 17 * 60)); // after its end
    CHECK(in_window(day, 2, 21 * 60));       // sport, no end: until the night routine
    CHECK(in_window(day, 3, 15));            // 0:15
    CHECK_FALSE(in_window(day, 3, 45));      // 0:45, after 0:30
    CHECK_FALSE(in_window(day, 0, 7 * 60));  // before it begins
}

TEST_CASE("a routine may end exactly at the day change") {
    auto shift = routine("Late shift", 22 * 60);
    shift.end = 4 * 60;
    const std::vector<Routine> day{routine("Morning", 8 * 60), shift};
    CHECK(in_window(day, 1, 3 * 60 + 59));
    CHECK(in_window(day, 1, 23 * 60));
}
