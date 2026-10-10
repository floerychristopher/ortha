#include "core/goal_repository.hpp"
#include "test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;
using namespace std::chrono;

namespace {

Goal make_goal(std::string title, Date start,
               Weekdays days = {true, true, true, true, true, true, true}) {
    Goal g;
    g.title = std::move(title);
    g.start = start;
    g.weekdays = days;
    return g;
}

constexpr Weekdays kMwf{true, false, true, false, true, false, false};

} // namespace

TEST_CASE("goal round trip") {
    Database db(":memory:");
    GoalRepository repo(db);
    auto g = make_goal("Gym", 2026y / October / 5d, kMwf);
    repo.insert(g);
    CHECK(g.id.size() == 36);
    CHECK_FALSE(g.created_at.empty());

    auto f = repo.find(g.id);
    REQUIRE(f);
    CHECK(f->title == "Gym");
    CHECK(f->weekdays == kMwf);
    CHECK(f->start == Date{2026y, October, 5d});
}

TEST_CASE("update and soft delete") {
    Database db(":memory:");
    GoalRepository repo(db);
    auto g = make_goal("Gym", 2026y / October / 5d, kMwf);
    repo.insert(g);
    g.title = "Gym (evening)";
    g.weekdays = {true, true, true, true, true, true, true};
    repo.update(g);
    CHECK(repo.find(g.id)->title == "Gym (evening)");
    CHECK(repo.find(g.id)->weekdays == g.weekdays);

    repo.set_status(g.id, 2026y / October / 7d, GoalStatus::Done);
    repo.remove(g.id);
    CHECK_FALSE(repo.find(g.id));
    CHECK(scalar(db, "SELECT COUNT(*) FROM goals WHERE deleted = 0") == 0);
    CHECK(scalar(db, "SELECT COUNT(*) FROM goal_entries") == 1);
    CHECK(scalar(db, "SELECT deleted FROM goals WHERE id = ?", g.id) == 1);
}

TEST_CASE("goal symbol is stored and updated") {
    Database db(":memory:");
    GoalRepository repo(db);
    auto g = make_goal("Gym", 2026y / October / 5d, kMwf);
    g.symbol = "💪";
    repo.insert(g);
    CHECK(repo.find(g.id)->symbol == "💪");
    g.symbol = "📚4h";
    repo.update(g);
    CHECK(repo.find(g.id)->symbol == "📚4h");
}

TEST_CASE("set_status upserts one row per day") {
    Database db(":memory:");
    GoalRepository repo(db);
    auto g = make_goal("Read", 2026y / October / 1d);
    repo.insert(g);
    const Date day = 2026y / October / 7d;
    CHECK(repo.status(g.id, day) == GoalStatus::Open);
    repo.set_status(g.id, day, GoalStatus::Done);
    CHECK(repo.status(g.id, day) == GoalStatus::Done);
    repo.set_status(g.id, day, GoalStatus::Missed);
    repo.set_status(g.id, day, GoalStatus::Skipped);
    CHECK(repo.status(g.id, day) == GoalStatus::Skipped);
    repo.set_status(g.id, day, GoalStatus::Open);
    CHECK(repo.status(g.id, day) == GoalStatus::Open);
    CHECK(scalar(db, "SELECT COUNT(*) FROM goal_entries") == 1);
    CHECK(repo.status(g.id, 2026y / October / 6d) == GoalStatus::Open);
}

TEST_CASE("statuses returns stored entries in range") {
    Database db(":memory:");
    GoalRepository repo(db);
    auto g = make_goal("Read", 2026y / October / 1d);
    repo.insert(g);
    repo.set_status(g.id, 2026y / October / 1d, GoalStatus::Done);
    repo.set_status(g.id, 2026y / October / 5d, GoalStatus::Missed);
    repo.set_status(g.id, 2026y / October / 9d, GoalStatus::Done);
    auto entries = repo.statuses(g.id, 2026y / October / 2d, 2026y / October / 8d);
    REQUIRE(entries.size() == 1);
    CHECK(entries.at(2026y / October / 5d) == GoalStatus::Missed);
}
