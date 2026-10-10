#include "tui/goal_widgets.hpp"

#include <catch2/catch_test_macros.hpp>

#include <ftxui/screen/screen.hpp>

#include <regex>

using namespace ortha;

TEST_CASE("goal_symbol uses the symbol or the start of the title") {
    Goal g;
    g.title = "4h lernen";
    CHECK(goal_symbol(g) == "4h l");
    g.title = "Gym";
    CHECK(goal_symbol(g) == "Gym");
    g.title = "abc📚"; // the emoji would make it 5 columns wide
    CHECK(goal_symbol(g) == "abc");
    g.title = "Eat  fruit"; // no trailing space
    CHECK(goal_symbol(g) == "Eat");
    g.symbol = "💪";
    CHECK(goal_symbol(g) == "💪");
}

namespace {

std::string draw(ftxui::Element element, int width = 10) {
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width), ftxui::Dimension::Fixed(1));
    ftxui::Render(screen, element);
    return std::regex_replace(screen.ToString(), std::regex("\x1b\\[[0-9;?]*[A-Za-z]"), "");
}

} // namespace

TEST_CASE("habit_label shows the symbol only when there is one") {
    Goal g;
    g.title = "Pray";
    CHECK(habit_label(g) == "Pray");
    g.symbol = "🙏";
    CHECK(habit_label(g) == "🙏 Pray");
}

TEST_CASE("status_mark draws one character per status") {
    CHECK(draw(status_mark(GoalStatus::Done)).starts_with("✓"));
    CHECK(draw(status_mark(GoalStatus::Missed)).starts_with("✗"));
    CHECK(draw(status_mark(GoalStatus::Open)).starts_with("○"));
    CHECK(draw(status_mark(GoalStatus::Skipped)).starts_with("·")); // rest day = not planned
    CHECK(draw(status_mark(std::nullopt)).starts_with("·"));
}

TEST_CASE("streak_label is right-aligned and empty without a streak") {
    CHECK(draw(streak_label(12, 7), 7) == "  🔥 12");
    CHECK(draw(streak_label(0, 7), 7) == "       ");
}
