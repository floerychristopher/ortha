#include "tui/goal_form.hpp"
#include "render.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;
using namespace std::chrono;

TEST_CASE("apply_goal_form validates") {
    GoalFormState s;
    s.title = "  ";
    CHECK(std::get<std::string>(apply_goal_form(s, Goal{})) == "Title must not be empty");
    s.title = "Gym";
    s.weekdays = {};
    CHECK(std::get<std::string>(apply_goal_form(s, Goal{})) == "Choose at least one day");
}

TEST_CASE("apply_goal_form keeps identity and trims") {
    Goal base;
    base.id = "g1";
    base.start = 2026y / October / 1d;
    base.created_at = "2026-10-01T00:00:00Z";
    GoalFormState s{" Gym ", {true, false, true, false, true, false, false}};
    auto g = std::get<Goal>(apply_goal_form(s, base));
    CHECK(g.id == "g1");
    CHECK(g.start == Date{2026y, October, 1d});
    CHECK(g.created_at == "2026-10-01T00:00:00Z");
    CHECK(g.title == "Gym");
    CHECK(g.weekdays == s.weekdays);
}

TEST_CASE("goal_form_state_from copies the goal") {
    Goal g;
    g.title = "Read";
    g.weekdays = {false, false, false, false, false, true, true};
    auto s = goal_form_state_from(g);
    CHECK(s.title == "Read");
    CHECK(s.weekdays == g.weekdays);
}

TEST_CASE("goal form renders and saves on enter") {
    std::optional<Goal> saved;
    auto form = make_goal_form("New goal", Goal{}, [&](Goal g) { saved = g; }, [] {});
    auto out = render(form);
    for (auto s : {"New goal", "Title", "Days", "Mo", "Su", "Save", "Cancel"}) {
        INFO(s);
        CHECK(contains(out, s));
    }
    for (char c : std::string("Gym"))
        form->OnEvent(ftxui::Event::Character(c));
    form->OnEvent(ftxui::Event::Return);
    REQUIRE(saved);
    CHECK(saved->title == "Gym");
    CHECK(saved->weekdays == Weekdays{true, true, true, true, true, true, true});
}

TEST_CASE("goal form shows validation errors and cancels on escape") {
    bool cancelled = false;
    auto form = make_goal_form("New goal", Goal{}, [](Goal) {}, [&] { cancelled = true; });
    form->OnEvent(ftxui::Event::Return);
    CHECK(contains(render(form), "Title must not be empty"));
    form->OnEvent(ftxui::Event::Escape);
    CHECK(cancelled);
}
