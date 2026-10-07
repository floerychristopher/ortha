#include "core/schema.hpp"
#include "core/task_repository.hpp"
#include "test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;

TEST_CASE("fresh database is migrated to the newest schema") {
    Database db(":memory:");
    migrate(db);
    CHECK(db.user_version() == 2);
    CHECK(scalar(db, "SELECT COUNT(*) FROM sqlite_master "
                     "WHERE name IN ('goals', 'goal_entries')") == 2);
}

TEST_CASE("migrate is idempotent") {
    Database db(":memory:");
    migrate(db);
    CHECK_NOTHROW(migrate(db));
    CHECK(db.user_version() == 2);
}

TEST_CASE("v1 database keeps its tasks when upgraded") {
    Database db(":memory:");
    migrate(db, 1);
    CHECK(db.user_version() == 1);
    db.exec("INSERT INTO tasks (id, title, created_at, updated_at) "
            "VALUES ('t1', 'Old task', 'x', 'x')");
    TaskRepository repo(db); // migrates to v2
    CHECK(db.user_version() == 2);
    REQUIRE(repo.find("t1"));
    CHECK(repo.find("t1")->title == "Old task");
}

TEST_CASE("newer database is rejected") {
    Database db(":memory:");
    db.set_user_version(3);
    CHECK_THROWS_AS(migrate(db), SqliteError);
}
