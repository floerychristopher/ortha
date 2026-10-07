#include "tui/goal_history_view.hpp"
#include "render.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;
using namespace std::chrono;

namespace {

std::vector<HistoryDay> three_days() {
    return {{2026y / October / 7d, GoalStatus::Done},
            {2026y / October / 5d, GoalStatus::Missed},
            {2026y / October / 2d, GoalStatus::Open}};
}

} // namespace

TEST_CASE("history lists days with status") {
    auto view = make_goal_history("Gym", [] { return three_days(); }, [](Date) {}, [] {});
    auto out = render(view);
    for (auto s : {"History: Gym", "We 07.10.2026", "[✓]", "Mo 05.10.2026", "[✗]",
                   "Fr 02.10.2026", "[ ]"}) {
        INFO(s);
        CHECK(contains(out, s));
    }
    CHECK(out.find("07.10.2026") < out.find("02.10.2026"));
}

TEST_CASE("enter cycles the selected day") {
    auto days = three_days();
    std::optional<Date> cycled;
    auto view = make_goal_history(
        "Gym", [&] { return days; },
        [&](Date day) {
            cycled = day;
            days[1].status = GoalStatus::Open;
        },
        [] {});
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::Return);
    REQUIRE(cycled);
    CHECK(*cycled == Date{2026y, October, 5d});
    CHECK_FALSE(contains(render(view), "[✗]"));
}

TEST_CASE("escape leaves history") {
    bool back = false;
    auto view = make_goal_history("Gym", [] { return three_days(); }, [](Date) {},
                                  [&] { back = true; });
    view->OnEvent(ftxui::Event::Escape);
    CHECK(back);
}

TEST_CASE("history without scheduled days shows a placeholder") {
    auto view = make_goal_history("Later", [] { return std::vector<HistoryDay>{}; },
                                  [](Date) {}, [] {});
    auto out = render(view);
    CHECK(contains(out, "No days yet."));
    CHECK_FALSE(contains(out, "Empty container"));
}
