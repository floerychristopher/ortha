#include "core/sqlite.hpp"

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>

using namespace ortha;

TEST_CASE("insert and select with bound parameters") {
    Database db(":memory:");
    db.exec("CREATE TABLE t (a TEXT, b INTEGER)");
    db.prepare("INSERT INTO t VALUES (?, ?)").bind(1, "hällo").bind(2, std::int64_t{42}).step();
    db.prepare("INSERT INTO t VALUES (?, ?)").bind_null(1).bind(2, std::int64_t{7}).step();

    auto s = db.prepare("SELECT a, b FROM t ORDER BY b DESC");
    REQUIRE(s.step());
    CHECK(s.column_text(0) == "hällo");
    CHECK(s.column_int(1) == 42);
    REQUIRE(s.step());
    CHECK(s.column_is_null(0));
    CHECK_FALSE(s.step());
}

TEST_CASE("invalid sql throws SqliteError") {
    Database db(":memory:");
    CHECK_THROWS_AS(db.exec("NOT SQL"), SqliteError);
    CHECK_THROWS_AS(db.prepare("SELECT * FROM missing"), SqliteError);
}

TEST_CASE("transaction rolls back on exception, nested ok") {
    Database db(":memory:");
    db.exec("CREATE TABLE t (a INTEGER)");
    CHECK_THROWS(db.transaction([&] {
        db.exec("INSERT INTO t VALUES (1)");
        db.transaction([&] { db.exec("INSERT INTO t VALUES (2)"); });
        throw std::runtime_error("boom");
    }));
    auto s = db.prepare("SELECT COUNT(*) FROM t");
    s.step();
    CHECK(s.column_int(0) == 0);
}

TEST_CASE("transaction commits on success") {
    Database db(":memory:");
    db.exec("CREATE TABLE t (a INTEGER)");
    db.transaction([&] { db.exec("INSERT INTO t VALUES (1)"); });
    auto s = db.prepare("SELECT COUNT(*) FROM t");
    s.step();
    CHECK(s.column_int(0) == 1);
}

TEST_CASE("user_version round trip") {
    Database db(":memory:");
    CHECK(db.user_version() == 0);
    db.set_user_version(3);
    CHECK(db.user_version() == 3);
}

TEST_CASE("connections wait briefly for locks held by other connections") {
    Database db(":memory:");
    auto s = db.prepare("PRAGMA busy_timeout");
    s.step();
    CHECK(s.column_int(0) == 2000);
}
