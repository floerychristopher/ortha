#include "tui/goal_widgets.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;

TEST_CASE("goal_symbol uses the symbol or the start of the title") {
    Goal g;
    g.title = "4h lernen";
    CHECK(goal_symbol(g) == "4h l");
    g.title = "Gym";
    CHECK(goal_symbol(g) == "Gym");
    g.title = "abc📚"; // the emoji would make it 5 columns wide
    CHECK(goal_symbol(g) == "abc");
    g.symbol = "💪";
    CHECK(goal_symbol(g) == "💪");
}
