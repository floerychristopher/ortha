#include "core/task_service.hpp"
#include "test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;
using namespace std::chrono;

TEST_CASE("complete and reopen") {
    Database db(":memory:");
    TaskRepository repo(db);
    TaskService svc(repo);
    auto t = make_task("x");
    repo.insert(t);

    CHECK_FALSE(svc.complete(t.id, Date{2026y, October, 7d}));
    CHECK(repo.find(t.id)->done);
    CHECK(repo.find(t.id)->completed_at);

    CHECK_FALSE(svc.complete(t.id, Date{2026y, October, 7d})); // second time: no-op
    CHECK(repo.all(true).size() == 1);

    svc.reopen(t.id);
    CHECK_FALSE(repo.find(t.id)->done);
    CHECK_FALSE(repo.find(t.id)->completed_at);
}

TEST_CASE("unknown ids are ignored") {
    Database db(":memory:");
    TaskRepository repo(db);
    TaskService svc(repo);
    CHECK_FALSE(svc.complete("missing", Date{2026y, October, 7d}));
    CHECK_NOTHROW(svc.reopen("missing"));
}

namespace {
Task recurring_gym(RecurrenceRule rule, Date due) {
    auto t = make_task("Gym", due);
    t.tags = {"health"};
    t.subtasks = {{"", "Pack bag", true}};
    t.recurrence = rule;
    return t;
}
} // namespace

TEST_CASE("completing a recurring task creates the next one") {
    Database db(":memory:");
    TaskRepository repo(db);
    TaskService svc(repo);
    auto t = recurring_gym(RecurrenceRule{Frequency::Weekly}, Date{2026y, October, 7d});
    repo.insert(t);

    auto next = svc.complete(t.id, Date{2026y, October, 7d});
    REQUIRE(next);
    CHECK(next->id != t.id);
    CHECK(next->title == "Gym");
    CHECK(next->due == Date{2026y, October, 14d});
    CHECK(next->tags == t.tags);
    CHECK_FALSE(next->done);
    REQUIRE(next->subtasks.size() == 1);
    CHECK_FALSE(next->subtasks[0].done);
    CHECK(next->subtasks[0].id != t.subtasks[0].id);
    CHECK(next->recurrence == t.recurrence);

    auto stored_next = repo.find(next->id);
    REQUIRE(stored_next);
    CHECK(stored_next->due == Date{2026y, October, 14d});

    auto old = repo.find(t.id);
    CHECK(old->done);
    CHECK_FALSE(old->recurrence);
    CHECK(old->subtasks[0].done); // the completed instance keeps its checklist state
}

TEST_CASE("completion anchor uses today") {
    Database db(":memory:");
    TaskRepository repo(db);
    TaskService svc(repo);
    auto t = recurring_gym(RecurrenceRule{Frequency::Daily, 1, {}, Anchor::Completion},
                           Date{2026y, October, 1d});
    repo.insert(t);
    auto next = svc.complete(t.id, Date{2026y, October, 7d});
    REQUIRE(next);
    CHECK(next->due == Date{2026y, October, 8d});
}

TEST_CASE("complete, reopen, complete spawns only one follow-up") {
    Database db(":memory:");
    TaskRepository repo(db);
    TaskService svc(repo);
    auto t = recurring_gym(RecurrenceRule{Frequency::Daily}, Date{2026y, October, 7d});
    repo.insert(t);
    CHECK(svc.complete(t.id, Date{2026y, October, 7d}));
    svc.reopen(t.id);
    CHECK_FALSE(svc.complete(t.id, Date{2026y, October, 7d}));
    CHECK(repo.all(true).size() == 2);
}
