#include "tui/home_view.hpp"
#include "../core/test_helpers.hpp"
#include "render.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;
using namespace std::chrono;

namespace {

constexpr Date kToday{2026y, October, 7d};

std::vector<Task> three_tasks() {
    std::vector<Task> tasks{make_task("Pay taxes", Date{2026y, October, 6d}, Priority::High),
                            make_task("Buy milk", kToday),
                            make_task("Gym", Date{2026y, October, 9d})};
    for (std::size_t i = 0; i < tasks.size(); ++i)
        tasks[i].id = "id-" + std::to_string(i);
    return tasks;
}

GoalSummary goal_summary(std::string id, std::string title, GoalStatus today, int streak) {
    GoalSummary s;
    s.goal.id = std::move(id);
    s.goal.title = std::move(title);
    s.today = today;
    s.streak = streak;
    return s;
}

HomeOptions options_with_tasks(int open = 5) {
    HomeOptions o;
    o.load_tasks = [] { return three_tasks(); };
    o.count_open = [open] { return open; };
    o.today = kToday;
    return o;
}

std::size_t count(const std::string& haystack, const std::string& needle) {
    std::size_t n = 0;
    for (auto pos = haystack.find(needle); pos != std::string::npos;
         pos = haystack.find(needle, pos + 1))
        ++n;
    return n;
}

} // namespace

TEST_CASE("home shows today's tasks and the menu with the open count") {
    auto home = make_home(options_with_tasks(5));
    auto out = render(home);
    for (auto s : {"Today", "Pay taxes", "06.10.2026", "Buy milk", "Gym", "New task",
                   "All Tasks (5)", "Goals", "Quit"}) {
        INFO(s);
        CHECK(contains(out, s));
    }
}

TEST_CASE("home without tasks says so") {
    HomeOptions o;
    o.load_tasks = [] { return std::vector<Task>{}; };
    o.count_open = [] { return 0; };
    o.today = kToday;
    auto home = make_home(o);
    CHECK(contains(render(home), "Nothing to do."));
    CHECK(contains(render(home), "All Tasks (0)"));
}

TEST_CASE("home starts on 'New task' and the menu triggers its actions") {
    int new_count = 0, all_count = 0, goals_count = 0, quit_count = 0;
    auto o = options_with_tasks(3);
    o.actions = {.on_new = [&] { ++new_count; },
                 .on_all = [&] { ++all_count; },
                 .on_goals = [&] { ++goals_count; },
                 .on_quit = [&] { ++quit_count; }};
    auto home = make_home(o);
    home->OnEvent(ftxui::Event::Return);
    CHECK(new_count == 1);
    home->OnEvent(ftxui::Event::ArrowDown);
    home->OnEvent(ftxui::Event::Return);
    CHECK(all_count == 1);
    home->OnEvent(ftxui::Event::ArrowDown);
    home->OnEvent(ftxui::Event::Return);
    CHECK(goals_count == 1);
    home->OnEvent(ftxui::Event::ArrowDown);
    home->OnEvent(ftxui::Event::Return);
    CHECK(quit_count == 1);
}

TEST_CASE("tasks on home can be completed, the count updates") {
    auto tasks = three_tasks();
    int open = 3;
    std::string toggled;
    HomeOptions o;
    o.load_tasks = [&] { return tasks; };
    o.count_open = [&] { return open; };
    o.today = kToday;
    o.task_callbacks.on_toggle_done = [&](const std::string& id) {
        toggled = id;
        tasks.erase(tasks.begin() + 2);
        --open;
    };
    auto home = make_home(o);
    home->OnEvent(ftxui::Event::ArrowUp); // from the menu into the task rows (last row)
    home->OnEvent(ftxui::Event::Return);
    CHECK(contains(render(home), "Toggle done"));
    home->OnEvent(ftxui::Event::Return);
    CHECK(toggled == "id-2");
    auto out = render(home);
    CHECK(contains(out, "All Tasks (2)"));
    CHECK_FALSE(contains(out, "Gym"));
}

TEST_CASE("home shows today's goals above the tasks") {
    auto o = options_with_tasks();
    o.load_goals = [] {
        return std::vector<GoalSummary>{goal_summary("g1", "Workout", GoalStatus::Done, 5),
                                        goal_summary("g2", "4h lernen", GoalStatus::Open, 0)};
    };
    auto out = render(make_home(o));
    CHECK(contains(out, "[✓] Workout"));
    CHECK(contains(out, "🔥 5"));
    CHECK(contains(out, "[ ] 4h lernen"));
    CHECK_FALSE(contains(out, "🔥 0"));
    CHECK(out.find("Goals") < out.find("Pay taxes"));
}

TEST_CASE("home without goals has no goals section") {
    auto out = render(make_home(options_with_tasks()));
    CHECK(count(out, "Goals") == 1); // only the menu entry
}

TEST_CASE("enter on a goal cycles its status") {
    auto status = GoalStatus::Open;
    std::string cycled;
    auto o = options_with_tasks();
    o.load_goals = [&] {
        return std::vector<GoalSummary>{goal_summary("g1", "Workout", status, 0)};
    };
    o.on_cycle_goal = [&](const std::string& id) {
        cycled = id;
        status = GoalStatus::Done;
    };
    auto home = make_home(o);
    for (int i = 0; i < 4; ++i) // menu -> 3 task rows -> goals
        home->OnEvent(ftxui::Event::ArrowUp);
    home->OnEvent(ftxui::Event::Return);
    CHECK(cycled == "g1");
    CHECK(contains(render(home), "[✓] Workout"));
}

TEST_CASE("long goal titles keep the streak visible") {
    auto o = options_with_tasks();
    o.load_goals = [] {
        return std::vector<GoalSummary>{
            goal_summary("g1", std::string(100, 'x'), GoalStatus::Open, 3)};
    };
    CHECK(contains(render(make_home(o), 50, 24), "🔥 3"));
}

TEST_CASE("home shows the bible verse below the menu") {
    auto o = options_with_tasks();
    o.verse = BibleVerse{"Psalm 23,4", "Und ob ich schon wanderte im finstern Tal, fürchte ich "
                                       "kein Unglück; denn du bist bei mir"};
    auto out = render(make_home(o), 80, 30);
    CHECK(contains(out, "Psalm 23,4"));
    CHECK(contains(out, "finstern Tal"));
    CHECK(out.find("Quit") < out.find("finstern Tal"));
}

TEST_CASE("home without a verse shows none") {
    auto out = render(make_home(options_with_tasks()));
    CHECK_FALSE(contains(out, "Psalm"));
}
