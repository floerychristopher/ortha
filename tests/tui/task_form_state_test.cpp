#include "tui/task_form_state.hpp"
#include "../core/test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;
using namespace std::chrono;

TEST_CASE("form_state_from maps task fields") {
    auto t = make_task("Pay rent", Date{2026y, October, 7d}, Priority::Medium);
    t.tags = {"home", "money"};
    t.notes = "IBAN in mail";
    auto s = form_state_from(t);
    CHECK(s.title == "Pay rent");
    CHECK(s.due == "07.10.2026");
    CHECK(s.tags == "home, money");
    CHECK(s.notes == "IBAN in mail");
    CHECK(s.priority == 2);
}

TEST_CASE("form_state_from leaves due empty without date") {
    CHECK(form_state_from(make_task("x")).due.empty());
}

TEST_CASE("apply_form rejects blank title") {
    TaskFormState s;
    s.title = "   ";
    CHECK(std::get<std::string>(apply_form(s, Task{})) == "Title must not be empty");
}

TEST_CASE("apply_form rejects invalid due date") {
    TaskFormState s;
    s.title = "x";
    s.due = "31.02.2026";
    CHECK(std::get<std::string>(apply_form(s, Task{})) == "Due date must be DD.MM.YYYY");
}

TEST_CASE("apply_form builds task and keeps identity") {
    Task base = make_task("old");
    base.id = "id-1";
    base.created_at = "2026-10-01T00:00:00Z";
    TaskFormState s{" New title ", "", "B, a", "note", 3};
    auto t = std::get<Task>(apply_form(s, base));
    CHECK(t.id == "id-1");
    CHECK(t.created_at == "2026-10-01T00:00:00Z");
    CHECK(t.title == "New title");
    CHECK_FALSE(t.due);
    CHECK(t.tags == std::vector<std::string>{"a", "b"});
    CHECK(t.notes == "note");
    CHECK(t.priority == Priority::High);
}

TEST_CASE("apply_form parses due date") {
    TaskFormState s;
    s.title = "x";
    s.due = "7.10.2026";
    CHECK(std::get<Task>(apply_form(s, Task{})).due == Date{2026y, October, 7d});
}

TEST_CASE("form state copies subtasks") {
    auto t = make_task("Move");
    t.subtasks = {{"a", "Pack", true}};
    auto s = form_state_from(t);
    REQUIRE(s.subtasks.size() == 1);
    CHECK(s.subtasks[0].title == "Pack");
}

TEST_CASE("apply_form keeps subtasks and drops blank ones") {
    TaskFormState s;
    s.title = "x";
    s.subtasks = {{"", "Step 1", true}, {"", "   ", false}};
    auto t = std::get<Task>(apply_form(s, Task{}));
    REQUIRE(t.subtasks.size() == 1);
    CHECK(t.subtasks[0].title == "Step 1");
    CHECK(t.subtasks[0].done);
}

TEST_CASE("form maps recurrence both ways") {
    TaskFormState s;
    s.title = "Gym";
    s.due = "07.10.2026";
    s.repeat = 2;
    s.interval = "2";
    s.weekdays = {true, false, true, false, true, false, false};
    s.anchor = 1;
    auto t = std::get<Task>(apply_form(s, Task{}));
    CHECK(t.recurrence ==
          RecurrenceRule{Frequency::Weekly, 2, {Monday, Wednesday, Friday}, Anchor::Completion});

    auto back = form_state_from(t);
    CHECK(back.repeat == 2);
    CHECK(back.interval == "2");
    CHECK(back.weekdays == s.weekdays);
    CHECK(back.anchor == 1);
}

TEST_CASE("weekdays are ignored unless weekly") {
    TaskFormState s;
    s.title = "Rent";
    s.due = "01.10.2026";
    s.repeat = 3;
    s.weekdays = {true, false, false, false, false, false, false};
    auto t = std::get<Task>(apply_form(s, Task{}));
    CHECK(t.recurrence == RecurrenceRule{Frequency::Monthly, 1, {}, Anchor::Due});
}

TEST_CASE("repeat validation") {
    TaskFormState s;
    s.title = "x";
    s.repeat = 1;
    CHECK(std::get<std::string>(apply_form(s, Task{})) == "Repeat needs a due date");
    s.due = "07.10.2026";
    for (auto bad : {"0", "1000", "abc", ""}) {
        INFO(bad);
        s.interval = bad;
        CHECK(std::get<std::string>(apply_form(s, Task{})) ==
              "Interval must be a number from 1 to 999");
    }
    s.repeat = 0; // no repeat: interval is ignored
    CHECK_FALSE(std::get<Task>(apply_form(s, Task{})).recurrence);
}
