#include "core/task_repository.hpp"
#include "test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;
using namespace std::chrono;

TEST_CASE("parse_tags normalizes") {
    CHECK(parse_tags(" Work, home ,work,, ") == std::vector<std::string>{"home", "work"});
    CHECK(parse_tags("").empty());
    CHECK(parse_tags("  Side Project ") == std::vector<std::string>{"side project"});
    CHECK(join_tags({"home", "work"}) == "home, work");
}

TEST_CASE("insert then find returns same task") {
    Database db(":memory:");
    TaskRepository repo(db);
    auto t = make_task("Buy milk", Date{2026y, October, 7d}, Priority::High);
    t.notes = "2 liters";
    t.tags = {"home"};
    t.subtasks = {{"", "Check fridge", true}};
    repo.insert(t);

    CHECK(t.id.size() == 36);
    CHECK_FALSE(t.created_at.empty());
    CHECK(t.updated_at == t.created_at);

    auto f = repo.find(t.id);
    REQUIRE(f);
    CHECK(f->title == "Buy milk");
    CHECK(f->notes == "2 liters");
    CHECK(f->due == t.due);
    CHECK(f->priority == Priority::High);
    CHECK(f->tags == t.tags);
    CHECK_FALSE(f->done);
    REQUIRE(f->subtasks.size() == 1);
    CHECK(f->subtasks[0].title == "Check fridge");
    CHECK(f->subtasks[0].done);
    CHECK(f->subtasks[0].id.size() == 36);
}

TEST_CASE("unicode and quotes survive round trip") {
    Database db(":memory:");
    TaskRepository repo(db);
    auto t = make_task("Müll 'rausbringen' \"jetzt\" 🗑");
    t.notes = "Zeile1\nZeile2";
    repo.insert(t);
    CHECK(repo.find(t.id)->title == t.title);
    CHECK(repo.find(t.id)->notes == t.notes);
}

TEST_CASE("update replaces fields, tags and subtasks") {
    Database db(":memory:");
    TaskRepository repo(db);
    auto t = make_task("Old");
    t.tags = {"a"};
    t.subtasks = {{"", "one", false}, {"", "two", false}};
    repo.insert(t);

    t.title = "New";
    t.tags = {"b"};
    t.done = true;
    t.completed_at = "2026-10-07T10:00:00Z";
    t.subtasks.erase(t.subtasks.begin());
    t.subtasks.push_back({"", "three", true});
    repo.update(t);

    auto f = repo.find(t.id);
    REQUIRE(f);
    CHECK(f->title == "New");
    CHECK(f->tags == std::vector<std::string>{"b"});
    CHECK(f->done);
    CHECK(f->completed_at == "2026-10-07T10:00:00Z");
    REQUIRE(f->subtasks.size() == 2);
    CHECK(f->subtasks[0].title == "two");
    CHECK(f->subtasks[1].title == "three");
    CHECK(scalar(db, "SELECT COUNT(*) FROM subtasks WHERE deleted = 1") == 1);
}

TEST_CASE("remove is a soft delete") {
    Database db(":memory:");
    TaskRepository repo(db);
    auto t = make_task("Gone soon");
    repo.insert(t);
    repo.remove(t.id);
    CHECK_FALSE(repo.find(t.id));
    CHECK(repo.all(true).empty());
    CHECK(scalar(db, "SELECT deleted FROM tasks WHERE id = ?", t.id) == 1);
}

TEST_CASE("all orders by done, due, priority") {
    Database db(":memory:");
    TaskRepository repo(db);
    auto a = make_task("A", Date{2026y, October, 10d}, Priority::Low);
    auto b = make_task("B", Date{2026y, October, 8d});
    auto c = make_task("C", std::nullopt, Priority::High);
    auto d = make_task("D", Date{2026y, October, 8d}, Priority::High);
    auto e = make_task("E", Date{2026y, October, 1d});
    e.done = true;
    for (auto* t : {&a, &b, &c, &d, &e})
        repo.insert(*t);

    CHECK(titles(repo.all(false)) == std::vector<std::string>{"D", "B", "A", "C"});
    CHECK(titles(repo.all(true)) == std::vector<std::string>{"D", "B", "A", "C", "E"});
}

TEST_CASE("upcoming returns the next open tasks, dated ones first") {
    Database db(":memory:");
    TaskRepository repo(db);
    auto late = make_task("Late", Date{2026y, December, 1d});
    auto overdue = make_task("Overdue", Date{2026y, September, 30d});
    auto soon_low = make_task("Soon low", Date{2026y, October, 8d}, Priority::Low);
    auto soon_high = make_task("Soon high", Date{2026y, October, 8d}, Priority::High);
    auto undated_high = make_task("Undated high", std::nullopt, Priority::High);
    auto undated_low = make_task("Undated low", std::nullopt, Priority::Low);
    auto done = make_task("Done", Date{2026y, September, 1d});
    done.done = true;
    for (auto* t : {&late, &undated_low, &overdue, &soon_low, &undated_high, &soon_high, &done})
        repo.insert(*t);

    CHECK(titles(repo.upcoming(3)) ==
          std::vector<std::string>{"Overdue", "Soon high", "Soon low"});
    CHECK(titles(repo.upcoming(5)) ==
          std::vector<std::string>{"Overdue", "Soon high", "Soon low", "Late", "Undated high"});
}

TEST_CASE("upcoming fills up with undated tasks and returns fewer if there are fewer") {
    Database db(":memory:");
    TaskRepository repo(db);
    auto dated = make_task("Dated", Date{2026y, October, 8d});
    auto undated = make_task("Undated");
    repo.insert(dated);
    repo.insert(undated);
    CHECK(titles(repo.upcoming(3)) == std::vector<std::string>{"Dated", "Undated"});
}

TEST_CASE("count_open counts only open, not deleted tasks") {
    Database db(":memory:");
    TaskRepository repo(db);
    auto open = make_task("Open");
    auto done = make_task("Done");
    done.done = true;
    auto removed = make_task("Removed");
    for (auto* t : {&open, &done, &removed})
        repo.insert(*t);
    repo.remove(removed.id);
    CHECK(repo.count_open() == 1);
}

TEST_CASE("migration is idempotent") {
    Database db(":memory:");
    {
        TaskRepository repo(db);
        auto t = make_task("Persisted");
        repo.insert(t);
    }
    TaskRepository again(db);
    CHECK(titles(again.all(false)) == std::vector<std::string>{"Persisted"});
    CHECK(db.user_version() == 1);
}

TEST_CASE("rejects database from newer version") {
    Database db(":memory:");
    db.set_user_version(99);
    CHECK_THROWS_AS(TaskRepository(db), SqliteError);
}

TEST_CASE("recurrence persists") {
    Database db(":memory:");
    TaskRepository repo(db);
    auto t = make_task("Gym", Date{2026y, October, 7d});
    t.recurrence = RecurrenceRule{Frequency::Weekly, 2, {Monday}, Anchor::Completion};
    repo.insert(t);
    CHECK(repo.find(t.id)->recurrence == t.recurrence);

    t.recurrence.reset();
    repo.update(t);
    CHECK_FALSE(repo.find(t.id)->recurrence);
}

TEST_CASE("unreadable recurrence loads as none") {
    Database db(":memory:");
    TaskRepository repo(db);
    auto t = make_task("x");
    repo.insert(t);
    db.prepare("UPDATE tasks SET recurrence = 'FREQ=SOMETIMES' WHERE id = ?").bind(1, t.id).step();
    CHECK_FALSE(repo.find(t.id)->recurrence);
}
