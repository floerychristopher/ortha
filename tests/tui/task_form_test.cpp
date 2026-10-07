#include "tui/task_form.hpp"
#include "../core/test_helpers.hpp"
#include "render.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;
using namespace std::chrono;

TEST_CASE("task form shows labels and prefilled values") {
    auto form = make_task_form("Edit task", make_task("Pay rent", Date{2026y, October, 7d}),
                               [](Task) {}, [] {});
    auto out = render(form);
    for (auto s : {"Edit task", "Title", "Due", "Priority", "Tags", "Notes", "Save", "Cancel",
                   "Pay rent", "07.10.2026"}) {
        INFO(s);
        CHECK(contains(out, s));
    }
}

TEST_CASE("escape cancels the form") {
    bool cancelled = false;
    auto form = make_task_form("New task", Task{}, [](Task) {}, [&] { cancelled = true; });
    form->OnEvent(ftxui::Event::Escape);
    CHECK(cancelled);
}

TEST_CASE("typing a title and pressing enter saves") {
    std::optional<Task> saved;
    auto form = make_task_form("New task", Task{}, [&](Task t) { saved = t; }, [] {});
    for (char c : std::string("Milk"))
        form->OnEvent(ftxui::Event::Character(c));
    form->OnEvent(ftxui::Event::Return);
    REQUIRE(saved);
    CHECK(saved->title == "Milk");
}

TEST_CASE("enter with blank title shows the error instead of saving") {
    bool saved = false;
    auto form = make_task_form("New task", Task{}, [&](Task) { saved = true; }, [] {});
    form->OnEvent(ftxui::Event::Return);
    CHECK_FALSE(saved);
    CHECK(contains(render(form), "Title must not be empty"));
}

TEST_CASE("editing appends to the existing title") {
    std::optional<Task> saved;
    auto form = make_task_form("Edit task", make_task("Pay rent"), [&](Task t) { saved = t; },
                               [] {});
    form->OnEvent(ftxui::Event::Character('!'));
    form->OnEvent(ftxui::Event::Return);
    REQUIRE(saved);
    CHECK(saved->title == "Pay rent!");
}

TEST_CASE("form lists subtasks") {
    auto t = make_task("Move");
    t.subtasks = {{"a", "Pack boxes", true}, {"b", "Rent van", false}};
    auto out = render(make_task_form("Edit task", t, [](Task) {}, [] {}), 80, 30);
    CHECK(contains(out, "Subtasks"));
    CHECK(contains(out, "Pack boxes"));
    CHECK(contains(out, "Rent van"));
    CHECK(contains(out, "Remove"));
}

namespace {
void type(const ftxui::Component& c, const std::string& text) {
    for (char ch : text)
        c->OnEvent(ftxui::Event::Character(ch));
}
} // namespace

TEST_CASE("tab moves to the next field, also past the priority toggle") {
    std::optional<Task> saved;
    auto form = make_task_form("New task", Task{}, [&](Task t) { saved = t; }, [] {});
    type(form, "Milk");
    form->OnEvent(ftxui::Event::Tab); // -> Due
    type(form, "07.10.2026");
    form->OnEvent(ftxui::Event::Tab); // -> Priority
    form->OnEvent(ftxui::Event::Tab); // -> Tags
    type(form, "home");
    form->OnEvent(ftxui::Event::Return);
    REQUIRE(saved);
    CHECK(saved->title == "Milk");
    CHECK(saved->due == Date{2026y, October, 7d});
    CHECK(saved->priority == Priority::None);
    CHECK(saved->tags == std::vector<std::string>{"home"});
}

TEST_CASE("form without subtasks shows no placeholder text") {
    auto out = render(make_task_form("New task", Task{}, [](Task) {}, [] {}), 80, 30);
    CHECK_FALSE(contains(out, "Empty container"));
}

TEST_CASE("repeat options appear only when repeating") {
    auto plain = render(make_task_form("New task", Task{}, [](Task) {}, [] {}), 90, 30);
    CHECK(contains(plain, "Repeat"));
    CHECK_FALSE(contains(plain, "Every"));

    auto t = make_task("Gym", Date{2026y, October, 7d});
    t.recurrence = RecurrenceRule{Frequency::Weekly, 1, {Monday}, Anchor::Due};
    auto weekly = render(make_task_form("Edit task", t, [](Task) {}, [] {}), 90, 30);
    CHECK(contains(weekly, "Every"));
    CHECK(contains(weekly, "Mo"));
    CHECK(contains(weekly, "From"));
}

TEST_CASE("subtask rows and weekday boxes are spaced out") {
    auto t = make_task("Gym", Date{2026y, October, 7d});
    t.subtasks = {{"a", "Pack", false}};
    t.recurrence = RecurrenceRule{Frequency::Weekly, 1, {Monday}, Anchor::Due};
    auto out = render(make_task_form("Edit task", t, [](Task) {}, [] {}), 90, 30);
    CHECK_FALSE(contains(out, "Pack Remove"));
    CHECK_FALSE(contains(out, "Mo☐"));
}

TEST_CASE("task form placeholders and choices are lowercase") {
    auto out = render(make_task_form("New task", Task{}, [](Task) {}, [] {}));
    for (auto s : {"what needs to be done?", "dd.mm.yyyy", "comma separated",
                   "notes (enter for new line)", "add subtask (enter)", "none│low│medium│high",
                   "none│daily│weekly│monthly│yearly"}) {
        INFO(s);
        CHECK(contains(out, s));
    }
}
