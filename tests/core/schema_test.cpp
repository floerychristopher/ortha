#include "core/goal_repository.hpp"
#include "core/schema.hpp"
#include "core/task_repository.hpp"
#include "test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;
using namespace std::chrono;

TEST_CASE("fresh database is migrated to the newest schema") {
    Database db(":memory:");
    migrate(db);
    CHECK(db.user_version() == 4);
    CHECK(scalar(db, "SELECT COUNT(*) FROM sqlite_master "
                     "WHERE name IN ('goals', 'goal_entries')") == 2);
}

TEST_CASE("migrate is idempotent") {
    Database db(":memory:");
    migrate(db);
    CHECK_NOTHROW(migrate(db));
    CHECK(db.user_version() == 4);
}

TEST_CASE("v1 database keeps its tasks when upgraded") {
    Database db(":memory:");
    migrate(db, 1);
    CHECK(db.user_version() == 1);
    db.exec("INSERT INTO tasks (id, title, created_at, updated_at) "
            "VALUES ('t1', 'Old task', 'x', 'x')");
    TaskRepository repo(db); // migrates to the newest schema
    CHECK(db.user_version() == 4);
    REQUIRE(repo.find("t1"));
    CHECK(repo.find("t1")->title == "Old task");
}

TEST_CASE("newer database is rejected") {
    Database db(":memory:");
    db.set_user_version(5);
    CHECK_THROWS_AS(migrate(db), SqliteError);
}

TEST_CASE("v2 database keeps its goals when upgraded") {
    Database db(":memory:");
    migrate(db, 2);
    db.exec("INSERT INTO goals (id, title, weekdays, start_date, created_at, updated_at) "
            "VALUES ('g1', 'Gym', 21, '2026-10-05', 'x', 'x')");
    db.exec("INSERT INTO goal_entries VALUES ('g1', '2026-10-05', 'done', 'x')");
    GoalRepository repo(db); // migrates to the newest schema
    CHECK(db.user_version() == 4);
    REQUIRE(repo.find("g1"));
    CHECK(repo.find("g1")->title == "Gym");
    CHECK(repo.find("g1")->symbol.empty());
    CHECK(repo.status("g1", 2026y / October / 5d) == GoalStatus::Done);
}
