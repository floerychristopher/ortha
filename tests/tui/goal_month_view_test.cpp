#include "tui/goal_month_view.hpp"
#include "render.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;
using namespace std::chrono;

// 07.10.2026 is a Wednesday.

namespace {

constexpr Date kToday = 2026y / October / 7d;

std::vector<MonthColumn> two_columns() {
    MonthColumn gym;
    gym.goal.id = "g1";
    gym.goal.title = "Gym";
    gym.goal.symbol = "💪";
    gym.goal.weekdays = {true, false, true, false, true, false, false};
    gym.goal.start = 2026y / October / 5d;
    gym.entries = {{2026y / October / 5d, GoalStatus::Done},
                   {2026y / October / 7d, GoalStatus::Missed}};

    MonthColumn read;
    read.goal.id = "g2";
    read.goal.title = "Read books";
    read.goal.start = 2026y / October / 1d;
    read.entries = {{2026y / October / 6d, GoalStatus::Done}};
    return {gym, read};
}

std::string render_month(const ftxui::Component& view) { return render(view, 80, 50); }

struct Calls {
    std::vector<std::pair<std::string, Date>> cycled;
    std::vector<year_month> loaded;
    bool back = false;
};

ftxui::Component make_view(Calls& calls) {
    return make_goal_month(
        [&calls](year_month month) {
            calls.loaded.push_back(month);
            return month == 2026y / October ? two_columns() : [] {
                auto columns = two_columns();
                for (auto& c : columns)
                    c.entries.clear();
                return columns;
            }();
        },
        kToday, [&calls](const std::string& id, Date day) { calls.cycled.emplace_back(id, day); },
        [&calls] { calls.back = true; });
}

} // namespace

TEST_CASE("month grid renders header, cells, footer and legend") {
    Calls calls;
    auto out = render_month(make_view(calls));
    for (auto s : {"October 2026", "< Sep", "Nov >", "💪", "Read", "Th 01", "Sa 31",
                   "💪 Gym · We 07.10.2026", "Read Read books"}) {
        INFO(s);
        CHECK(contains(out, s));
    }
    CHECK(contains(line_with(out, "Mo 05"), "✓"));
    CHECK(contains(line_with(out, "We 07"), "✗"));
    CHECK(contains(line_with(out, "Tu 06"), "·")); // Gym is not planned on Tuesdays
    CHECK(contains(line_with(out, "Th 01"), "·")); // Gym did not exist yet
    CHECK(contains(line_with(out, "Th 01"), "○")); // Read is open
    CHECK_FALSE(contains(line_with(out, "Th 08"), "○")); // the future stays empty
    CHECK_FALSE(contains(line_with(out, "Th 08"), "·"));
    CHECK(calls.loaded == std::vector<year_month>{2026y / October});
}

TEST_CASE("enter cycles the focused cell") {
    Calls calls;
    auto view = make_view(calls);
    view->OnEvent(ftxui::Event::Return);
    view->OnEvent(ftxui::Event::ArrowRight);
    view->OnEvent(ftxui::Event::Return);
    REQUIRE(calls.cycled.size() == 2);
    CHECK(calls.cycled[0] == std::pair<std::string, Date>{"g1", kToday});
    CHECK(calls.cycled[1] == std::pair<std::string, Date>{"g2", kToday});
    CHECK(calls.loaded.size() == 3); // reloaded after each change
    CHECK(contains(render_month(view), "Read books · We 07.10.2026"));
}

TEST_CASE("enter does nothing on unscheduled or future cells") {
    Calls calls;
    auto view = make_view(calls);
    view->OnEvent(ftxui::Event::ArrowUp); // Tu 06: Gym is not planned
    view->OnEvent(ftxui::Event::Return);
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::ArrowDown); // Th 08: future
    view->OnEvent(ftxui::Event::Return);
    CHECK(calls.cycled.empty());
    CHECK(contains(render_month(view), "💪 Gym · Th 08.10.2026"));
}

TEST_CASE("page keys change the month") {
    Calls calls;
    auto view = make_view(calls);
    view->OnEvent(ftxui::Event::PageUp);
    CHECK(contains(render_month(view), "September 2026"));
    CHECK(calls.loaded.back() == 2026y / September);
    view->OnEvent(ftxui::Event::PageDown);
    view->OnEvent(ftxui::Event::PageDown);
    auto out = render_month(view);
    CHECK(contains(out, "November 2026"));
    CHECK(contains(out, "< Oct"));
    CHECK(contains(out, "Dec >"));
    CHECK(calls.loaded.back() == 2026y / November);
}

TEST_CASE("changing to a shorter month keeps the cursor inside it") {
    Calls calls;
    auto view = make_view(calls);
    for (int i = 0; i < 24; ++i)
        view->OnEvent(ftxui::Event::ArrowDown); // 31.10.
    CHECK(contains(render_month(view), "31.10.2026"));
    view->OnEvent(ftxui::Event::PageDown);
    CHECK(contains(render_month(view), "💪 Gym · Mo 30.11.2026"));
}

TEST_CASE("arrow up from the first row moves into the toolbar") {
    Calls calls;
    auto view = make_view(calls);
    for (int i = 0; i < 7; ++i)
        view->OnEvent(ftxui::Event::ArrowUp); // 6x to the 1st, once more into the toolbar
    view->OnEvent(ftxui::Event::Return);       // "Back" is the first button
    CHECK(calls.back);
    CHECK(calls.cycled.empty());
}

TEST_CASE("escape leaves the grid") {
    Calls calls;
    auto view = make_view(calls);
    view->OnEvent(ftxui::Event::Escape);
    CHECK(calls.back);
}

TEST_CASE("month grid without goals") {
    bool back = false;
    auto view = make_goal_month([](year_month) { return std::vector<MonthColumn>{}; }, kToday,
                                [](const std::string&, Date) {}, [&] { back = true; });
    auto out = render_month(view);
    CHECK(contains(out, "No goals yet."));
    CHECK_FALSE(contains(out, "Empty container"));
    view->OnEvent(ftxui::Event::Escape);
    CHECK(back);
}

TEST_CASE("column symbols stay visible when the grid scrolls") {
    const Date late = 2026y / October / 28d;
    auto view = make_goal_month([](year_month) { return two_columns(); }, late,
                                [](const std::string&, Date) {}, [] {});
    auto out = render(view, 80, 24); // too small for all 31 days
    CHECK(contains(out, "We 28"));   // the cursor row is scrolled into view ...
    CHECK_FALSE(contains(out, "Th 01"));
    INFO(out);
    CHECK(contains(line_with(out, "💪"), "Read")); // ... and the header row is still there
}
