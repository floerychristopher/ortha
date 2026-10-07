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
                            make_task("Buy milk", kToday), make_task("Gym", Date{2026y, October, 9d})};
    for (std::size_t i = 0; i < tasks.size(); ++i)
        tasks[i].id = "id-" + std::to_string(i);
    return tasks;
}

} // namespace

TEST_CASE("home shows today's tasks and the menu with the open count") {
    auto home = make_home([] { return three_tasks(); }, [] { return 5; }, kToday, {}, {});
    auto out = render(home);
    for (auto s : {"Today", "Pay taxes", "06.10.2026", "Buy milk", "Gym", "New task",
                   "All Tasks (5)", "Quit"}) {
        INFO(s);
        CHECK(contains(out, s));
    }
}

TEST_CASE("home without tasks says so") {
    auto home = make_home([] { return std::vector<Task>{}; }, [] { return 0; }, kToday, {}, {});
    CHECK(contains(render(home), "Nothing to do."));
    CHECK(contains(render(home), "All Tasks (0)"));
}

TEST_CASE("home starts on 'New task' and the menu triggers its actions") {
    int new_count = 0, all_count = 0, quit_count = 0;
    auto home = make_home([] { return three_tasks(); }, [] { return 3; }, kToday,
                          {.on_new = [&] { ++new_count; },
                           .on_all = [&] { ++all_count; },
                           .on_quit = [&] { ++quit_count; }},
                          {});
    home->OnEvent(ftxui::Event::Return);
    CHECK(new_count == 1);
    home->OnEvent(ftxui::Event::ArrowDown);
    home->OnEvent(ftxui::Event::Return);
    CHECK(all_count == 1);
    home->OnEvent(ftxui::Event::ArrowDown);
    home->OnEvent(ftxui::Event::Return);
    CHECK(quit_count == 1);
}

TEST_CASE("tasks on home can be completed, the count updates") {
    auto tasks = three_tasks();
    int open = 3;
    std::string toggled;
    auto home = make_home([&] { return tasks; }, [&] { return open; }, kToday, {},
                          {.on_toggle_done = [&](const std::string& id) {
                              toggled = id;
                              tasks.erase(tasks.begin() + 2);
                              --open;
                          }});
    home->OnEvent(ftxui::Event::ArrowUp); // from the menu into the task rows (last row)
    home->OnEvent(ftxui::Event::Return);
    CHECK(contains(render(home), "Toggle done"));
    home->OnEvent(ftxui::Event::Return);
    CHECK(toggled == "id-2");
    auto out = render(home);
    CHECK(contains(out, "All Tasks (2)"));
    CHECK_FALSE(contains(out, "Gym"));
}
