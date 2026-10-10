#include "core/routine_repository.hpp"
#include "core/schema.hpp"
#include "test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;
using namespace std::chrono;

// 07.10.2026 is a Wednesday.

namespace {

constexpr Date kToday = 2026y / October / 7d;
constexpr Weekdays kMwf{true, false, true, false, true, false, false};

Routine make_routine(std::string name, int start) {
    Routine r;
    r.name = std::move(name);
    r.start = start;
    return r;
}

Goal make_habit(std::string title, const Routine& routine, Date start = 2026y / October / 1d,
                Weekdays days = {true, true, true, true, true, true, true}) {
    Goal g;
    g.title = std::move(title);
    g.routine_id = routine.id;
    g.start = start;
    g.weekdays = days;
    return g;
}

std::vector<std::string> habit_titles(const std::vector<Goal>& goals) {
    std::vector<std::string> out;
    for (const auto& g : goals)
        out.push_back(g.title);
    return out;
}

struct Repos {
    Database db{":memory:"};
    GoalRepository goals{db};
    RoutineRepository routines{db};
};

} // namespace

TEST_CASE("v3 database moves its goals into My routine") {
    Database db(":memory:");
    migrate(db, 3);
    db.exec("INSERT INTO goals (id, title, weekdays, start_date, created_at, updated_at, symbol) "
            "VALUES ('g2', 'Read', 127, '2026-10-02', '2026-10-02T10:00:00Z', 'x', '📚'), "
            "       ('g1', 'Gym', 21, '2026-10-01', '2026-10-01T10:00:00Z', 'x', '💪'), "
            "       ('g3', 'Gone', 127, '2026-10-01', '2026-10-01T09:00:00Z', 'x', '')");
    db.exec("UPDATE goals SET deleted = 1 WHERE id = 'g3'");
    db.exec("INSERT INTO goal_entries VALUES ('g1', '2026-10-05', 'done', 'x')");
    GoalRepository goals(db);
    RoutineRepository routines(db);
    CHECK(db.user_version() == 4);
    auto all = routines.all();
    REQUIRE(all.size() == 1);
    CHECK(all[0].name == "My routine");
    CHECK(all[0].start == 0);
    CHECK_FALSE(all[0].end);
    auto habits = goals.in_routine(all[0].id);
    CHECK(habit_titles(habits) == std::vector<std::string>{"Gym", "Read"}); // creation order
    CHECK(habits[0].position == 0);
    CHECK(habits[1].position == 1);
    CHECK(habits[0].symbol == "💪");
    CHECK(goals.status("g1", 2026y / October / 5d) == GoalStatus::Done);
}

TEST_CASE("fresh database has no routines") {
    Repos r;
    CHECK(r.db.user_version() == 4);
    CHECK(r.routines.all().empty());
}

TEST_CASE("routine round trip and order") {
    Repos r;
    auto night = make_routine("Night routine", 23 * 60);
    night.identity = "I end the day with God.";
    auto sport = make_routine("Sport", 19 * 60 + 30);
    sport.end = 21 * 60 + 30;
    auto morning = make_routine("Morning routine", 8 * 60);
    r.routines.insert(night);
    r.routines.insert(sport);
    r.routines.insert(morning);
    CHECK(night.id.size() == 36);

    auto found = r.routines.find(sport.id);
    REQUIRE(found);
    CHECK(found->name == "Sport");
    CHECK(found->start == 1170);
    CHECK(found->end == 1290);
    CHECK(r.routines.find(night.id)->identity == "I end the day with God.");

    std::vector<std::string> names;
    for (const auto& routine : r.routines.all())
        names.push_back(routine.name);
    CHECK(names == std::vector<std::string>{"Morning routine", "Sport", "Night routine"});

    sport.name = "Training";
    sport.end.reset();
    r.routines.update(sport);
    CHECK(r.routines.find(sport.id)->name == "Training");
    CHECK_FALSE(r.routines.find(sport.id)->end);

    auto second = make_routine("Second at 8", 8 * 60); // same start: after the older one
    r.routines.insert(second);
    CHECK(r.routines.all()[1].name == "Second at 8");
}

TEST_CASE("new habits go to the end, move swaps neighbours") {
    Repos r;
    auto morning = make_routine("Morning routine", 480);
    r.routines.insert(morning);
    auto a = make_habit("a", morning), b = make_habit("b", morning), c = make_habit("c", morning);
    r.goals.insert(a);
    r.goals.insert(b);
    r.goals.insert(c);
    CHECK(habit_titles(r.goals.in_routine(morning.id)) == std::vector<std::string>{"a", "b", "c"});
    r.goals.move(c.id, -1);
    CHECK(habit_titles(r.goals.in_routine(morning.id)) == std::vector<std::string>{"a", "c", "b"});
    r.goals.move(a.id, -1); // already first
    r.goals.move(b.id, +1); // already last
    CHECK(habit_titles(r.goals.in_routine(morning.id)) == std::vector<std::string>{"a", "c", "b"});
    r.goals.move(a.id, +1);
    CHECK(habit_titles(r.goals.in_routine(morning.id)) == std::vector<std::string>{"c", "a", "b"});
}

TEST_CASE("moving a habit to another routine appends it") {
    Repos r;
    auto morning = make_routine("Morning routine", 480), night = make_routine("Night", 1380);
    r.routines.insert(morning);
    r.routines.insert(night);
    auto pray = make_habit("Pray", morning), water = make_habit("Water", morning),
         read = make_habit("Read", night);
    r.goals.insert(pray);
    r.goals.insert(water);
    r.goals.insert(read);
    pray.routine_id = night.id;
    r.goals.update(pray);
    CHECK(habit_titles(r.goals.in_routine(night.id)) == std::vector<std::string>{"Read", "Pray"});
    CHECK(habit_titles(r.goals.in_routine(morning.id)) == std::vector<std::string>{"Water"});
    r.goals.move(water.id, -1); // the gap left behind does not get in the way
    CHECK(habit_titles(r.goals.in_routine(morning.id)) == std::vector<std::string>{"Water"});
}

TEST_CASE("removing a routine removes its habits") {
    Repos r;
    auto sport = make_routine("Sport", 1170), morning = make_routine("Morning", 480);
    r.routines.insert(sport);
    r.routines.insert(morning);
    auto gym = make_habit("Gym", sport), pray = make_habit("Pray", morning);
    r.goals.insert(gym);
    r.goals.insert(pray);
    r.goals.set_status(gym.id, kToday, GoalStatus::Done);
    r.routines.remove(sport.id);
    CHECK_FALSE(r.routines.find(sport.id));
    CHECK(r.routines.all().size() == 1);
    CHECK(r.goals.in_routine(sport.id).empty());
    CHECK(habit_titles(r.goals.in_routine(morning.id)) == std::vector<std::string>{"Pray"});
    CHECK(scalar(r.db, "SELECT COUNT(*) FROM goals WHERE deleted = 0") == 1);
    CHECK(scalar(r.db, "SELECT COUNT(*) FROM goal_entries") == 1);
}

TEST_CASE("summaries for today") {
    Repos r;
    auto morning = make_routine("Morning routine", 480), sport = make_routine("Sport", 1170);
    r.routines.insert(morning);
    r.routines.insert(sport);
    auto water = make_habit("Water", morning), pray = make_habit("Pray", morning);
    auto gym = make_habit("Gym", sport, 2026y / October / 1d, kMwf);
    for (auto* g : {&water, &pray, &gym})
        r.goals.insert(*g);
    for (auto d : {5d, 6d}) {
        r.goals.set_status(water.id, 2026y / October / d, GoalStatus::Done);
        r.goals.set_status(pray.id, 2026y / October / d, GoalStatus::Done);
    }
    r.goals.set_status(water.id, kToday, GoalStatus::Done);

    const auto tuesday = 2026y / October / 6d;
    auto today = r.routines.summaries(r.goals, tuesday, true);
    REQUIRE(today.size() == 1); // no gym on Tuesday
    CHECK(today[0].routine.name == "Morning routine");

    auto wednesday = r.routines.summaries(r.goals, kToday, true);
    REQUIRE(wednesday.size() == 2);
    const auto& m = wednesday[0];
    CHECK(habit_titles({m.habits[0].goal, m.habits[1].goal}) ==
          std::vector<std::string>{"Water", "Pray"});
    CHECK(m.today.planned == 2);
    CHECK(m.today.done == 1);
    CHECK(m.streak == 2);
    CHECK(m.habits[0].streak == 3);

    r.goals.set_status(pray.id, 2026y / October / 6d, GoalStatus::Missed);
    CHECK(r.routines.summaries(r.goals, kToday, true)[0].habits[1].missed_last_time);

    auto all = r.routines.summaries(r.goals, tuesday, false);
    REQUIRE(all.size() == 2);
    CHECK(all[1].habits.size() == 1); // gym, even though it is not planned today
}

TEST_CASE("moving an unknown habit does nothing") {
    Repos r;
    auto morning = make_routine("Morning routine", 480);
    r.routines.insert(morning);
    auto a = make_habit("a", morning), b = make_habit("b", morning);
    r.goals.insert(a);
    r.goals.insert(b);
    r.goals.move("no-such-id", -1);
    CHECK(habit_titles(r.goals.in_routine(morning.id)) == std::vector<std::string>{"a", "b"});
}

TEST_CASE("moving a habit without routine does nothing") {
    Repos r;
    auto morning = make_routine("Morning routine", 480);
    r.routines.insert(morning);
    auto loose = make_habit("loose", Routine{}); // no routine
    auto a = make_habit("a", morning);
    r.goals.insert(loose);
    r.goals.insert(a);
    r.goals.move(loose.id, -1);
    r.goals.move(loose.id, +1);
    CHECK(r.goals.find(loose.id)->position == 0);
    CHECK(habit_titles(r.goals.in_routine(morning.id)) == std::vector<std::string>{"a"});
}

TEST_CASE("removing a routine keeps the timestamps of habits deleted before") {
    Repos r;
    auto sport = make_routine("Sport", 1170);
    r.routines.insert(sport);
    auto gym = make_habit("Gym", sport), run = make_habit("Run", sport);
    r.goals.insert(gym);
    r.goals.insert(run);
    r.goals.remove(gym.id);
    r.db.exec("UPDATE goals SET updated_at = '2026-10-01T00:00:00Z' WHERE title = 'Gym'");
    r.routines.remove(sport.id);
    CHECK(scalar(r.db, "SELECT COUNT(*) FROM goals WHERE title = 'Gym' "
                       "AND updated_at = '2026-10-01T00:00:00Z'") == 1);
    CHECK(scalar(r.db, "SELECT deleted FROM goals WHERE title = 'Run'") == 1);
}
