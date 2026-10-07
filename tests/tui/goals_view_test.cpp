#include "tui/goals_view.hpp"
#include "render.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;

namespace {

constexpr auto Done = GoalStatus::Done;
constexpr auto Missed = GoalStatus::Missed;
constexpr auto Open = GoalStatus::Open;
constexpr std::optional<GoalStatus> No = std::nullopt;

std::vector<GoalSummary> two_goals() {
    GoalSummary gym;
    gym.goal.id = "g1";
    gym.goal.title = "Gym";
    gym.goal.weekdays = {true, false, true, false, true, false, false};
    gym.last_week = {No, Done, No, No, Done, No, Missed};
    gym.streak = 0;

    GoalSummary read;
    read.goal.id = "g2";
    read.goal.title = "Read";
    read.last_week = {Done, Done, Missed, Done, Done, Done, Open};
    read.streak = 3;
    return {gym, read};
}

} // namespace

TEST_CASE("goals view lists goals with days, week strip and streak") {
    auto view = make_goals_view([] { return two_goals(); }, {});
    auto out = render(view);
    for (auto s : {"Goals", "Back", "New goal", "Gym", "Mo We Fr", "·✓··✓·✗", "Read", "daily",
                   "✓✓✗✓✓✓○", "🔥 3"}) {
        INFO(s);
        CHECK(contains(out, s));
    }
    CHECK_FALSE(contains(out, "🔥 0"));
}

TEST_CASE("empty goals view") {
    bool back = false;
    auto view = make_goals_view([] { return std::vector<GoalSummary>{}; },
                                {.on_back = [&] { back = true; }});
    CHECK(contains(render(view), "No goals yet."));
    view->OnEvent(ftxui::Event::Escape);
    CHECK(back);
}

TEST_CASE("goal actions popup") {
    std::string history, edited;
    auto view = make_goals_view([] { return two_goals(); },
                                {.on_history = [&](const std::string& id) { history = id; },
                                 .on_edit = [&](const std::string& id) { edited = id; }});
    view->OnEvent(ftxui::Event::Return);
    auto out = render(view);
    CHECK(contains(out, "History"));
    CHECK(contains(out, "Edit"));
    CHECK(contains(out, "Delete"));
    view->OnEvent(ftxui::Event::Return);
    CHECK(history == "g1");

    view->OnEvent(ftxui::Event::Return);
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::Return);
    CHECK(edited == "g1");
}

TEST_CASE("deleting a goal asks for confirmation") {
    std::string deleted;
    auto view = make_goals_view([] { return two_goals(); },
                                {.on_delete = [&](const std::string& id) { deleted = id; }});
    view->OnEvent(ftxui::Event::Return);
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::Return);
    CHECK(deleted.empty());
    CHECK(contains(render(view), "Delete \"Gym\"?"));
    view->OnEvent(ftxui::Event::Return);
    CHECK(deleted == "g1");
}

TEST_CASE("escape closes the goal popup first") {
    bool back = false;
    auto view = make_goals_view([] { return two_goals(); }, {.on_back = [&] { back = true; }});
    view->OnEvent(ftxui::Event::Return);
    view->OnEvent(ftxui::Event::Escape);
    CHECK_FALSE(back);
    CHECK_FALSE(contains(render(view), "History"));
}
