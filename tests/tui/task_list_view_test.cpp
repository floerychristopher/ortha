#include "tui/task_list_view.hpp"
#include "../core/test_helpers.hpp"
#include "render.hpp"

#include <catch2/catch_test_macros.hpp>

#include <format>
#include <sstream>

using namespace ortha;
using namespace ortha::test;
using namespace std::chrono;

TEST_CASE("task list renders tasks and dates") {
    auto overdue = make_task("Overdue thing", Date{2026y, October, 1d}, Priority::High);
    overdue.tags = {"work"};
    std::vector<Task> tasks{overdue, make_task("No date")};
    auto list = make_task_list("Today", [&](bool) { return tasks; }, Date{2026y, October, 7d}, {});
    auto out = render(list);
    for (auto s : {"Today", "Overdue thing", "01.10.2026", "!!!", "#work", "No date",
                   "Show done: off", "Back"}) {
        INFO(s);
        CHECK(contains(out, s));
    }
}

TEST_CASE("task list says today and tomorrow instead of the date") {
    std::vector<Task> tasks{make_task("Due today", Date{2026y, October, 7d}),
                            make_task("Due tomorrow", Date{2026y, October, 8d})};
    auto out = render(
        make_task_list("All tasks", [&](bool) { return tasks; }, Date{2026y, October, 7d}, {}));
    CHECK(contains(line_with(out, "Due today"), "today"));
    CHECK(contains(line_with(out, "Due tomorrow"), "tomorrow"));
    CHECK_FALSE(contains(out, "07.10.2026"));
    CHECK_FALSE(contains(out, "08.10.2026"));
}

TEST_CASE("task list shows notes below each task") {
    auto taxes = make_task("Pay taxes");
    taxes.notes = "Receipts are in the 2025 folder\n\nDeadline per letter";
    std::vector<Task> tasks{taxes, make_task("Buy milk")};
    auto out = render(
        make_task_list("All tasks", [&](bool) { return tasks; }, Date{2026y, October, 7d}, {}));
    std::vector<std::string> lines;
    std::istringstream stream(out);
    for (std::string line; std::getline(stream, line);)
        lines.push_back(line);
    auto index_of = [&](const std::string& needle) {
        for (std::size_t i = 0; i < lines.size(); ++i)
            if (contains(lines[i], needle))
                return i;
        FAIL("missing " << needle);
        return lines.size();
    };
    const auto title = index_of("Pay taxes");
    CHECK(index_of("Receipts are in the 2025 folder") == title + 1);
    CHECK(index_of("Deadline per letter") == title + 2); // empty note lines are skipped
    CHECK(index_of("Buy milk") == title + 3);
    // indented to the title
    CHECK(lines[title + 1].find("Receipts") == lines[title].find("Pay taxes"));
}

TEST_CASE("done tasks are marked") {
    auto t = make_task("Finished");
    t.done = true;
    auto list = make_task_list("All tasks", [&](bool) { return std::vector<Task>{t}; },
                               Date{2026y, October, 7d}, {});
    CHECK(contains(render(list), "[x]"));
}

TEST_CASE("empty list shows placeholder, escape goes back") {
    bool back = false;
    auto list = make_task_list("All tasks", [](bool) { return std::vector<Task>{}; },
                               Date{2026y, October, 7d}, {.on_back = [&] { back = true; }});
    CHECK(contains(render(list), "nothing here."));
    list->OnEvent(ftxui::Event::Escape);
    CHECK(back);
}

TEST_CASE("list hides done tasks by default") {
    std::vector<bool> requests;
    auto list = make_task_list("All tasks",
                               [&](bool include_done) {
                                   requests.push_back(include_done);
                                   return std::vector<Task>{};
                               },
                               Date{2026y, October, 7d}, {});
    REQUIRE_FALSE(requests.empty());
    CHECK_FALSE(requests.back());
}

namespace {

ftxui::Component single_task_list(const Task& t, TaskListCallbacks callbacks) {
    return make_task_list("All tasks", [t](bool) { return std::vector<Task>{t}; },
                          Date{2026y, October, 7d}, std::move(callbacks));
}

Task buy_milk() {
    auto t = make_task("Buy milk");
    t.id = "id-1";
    return t;
}

} // namespace

TEST_CASE("enter opens actions, toggle calls back with id") {
    std::string toggled;
    auto list = single_task_list(buy_milk(), {.on_toggle_done = [&](auto& id) { toggled = id; }});
    list->OnEvent(ftxui::Event::Return);
    auto out = render(list);
    CHECK(contains(out, "Toggle done"));
    CHECK(contains(out, "Edit"));
    CHECK(contains(out, "Delete"));
    list->OnEvent(ftxui::Event::Return); // "Toggle done" is preselected
    CHECK(toggled == "id-1");
    CHECK_FALSE(contains(render(list), "Toggle done"));
}

TEST_CASE("edit action calls back with id") {
    std::string edited;
    auto list = single_task_list(buy_milk(), {.on_edit = [&](auto& id) { edited = id; }});
    list->OnEvent(ftxui::Event::Return);
    list->OnEvent(ftxui::Event::ArrowDown);
    list->OnEvent(ftxui::Event::Return);
    CHECK(edited == "id-1");
}

TEST_CASE("delete asks for confirmation") {
    std::string deleted;
    auto list = single_task_list(buy_milk(), {.on_delete = [&](auto& id) { deleted = id; }});
    list->OnEvent(ftxui::Event::Return);
    list->OnEvent(ftxui::Event::ArrowDown);
    list->OnEvent(ftxui::Event::ArrowDown);
    list->OnEvent(ftxui::Event::Return);
    CHECK(deleted.empty());
    CHECK(contains(render(list), "Delete \"Buy milk\"?"));
    list->OnEvent(ftxui::Event::Return); // "Delete" is preselected
    CHECK(deleted == "id-1");
}

TEST_CASE("escape closes the action menu instead of leaving the list") {
    bool back = false;
    auto list = single_task_list(buy_milk(), {.on_back = [&] { back = true; }});
    list->OnEvent(ftxui::Event::Return);
    list->OnEvent(ftxui::Event::Escape);
    CHECK_FALSE(back);
    CHECK_FALSE(contains(render(list), "Toggle done"));
}

TEST_CASE("done tasks offer reopen") {
    auto t = buy_milk();
    t.done = true;
    auto list = single_task_list(t, {});
    list->OnEvent(ftxui::Event::Return);
    CHECK(contains(render(list), "Reopen"));
}

TEST_CASE("list shows subtask progress") {
    auto t = make_task("Move");
    t.subtasks = {{"a", "Pack", true}, {"b", "Drive", false}};
    auto list = single_task_list(t, {});
    CHECK(contains(render(list), "1/2"));
}

TEST_CASE("list marks recurring tasks") {
    auto t = make_task("Gym", Date{2026y, October, 7d});
    t.recurrence = RecurrenceRule{};
    CHECK(contains(render(single_task_list(t, {})), "↻"));
}

TEST_CASE("long titles do not push the due date off screen") {
    auto t = make_task(std::string(120, 'x'), Date{2026y, October, 9d});
    t.tags = {"work"};
    auto out = render(single_task_list(t, {}), 60, 10);
    CHECK(contains(out, "09.10.2026"));
    CHECK(contains(out, "#work"));
}

TEST_CASE("list keeps the selected row in view while the action menu is open") {
    std::vector<Task> tasks;
    for (int i = 1; i <= 20; ++i) {
        auto t = make_task(std::format("Item {:02}", i));
        t.id = std::to_string(i);
        tasks.push_back(t);
    }
    auto list = make_task_list("All tasks", [&](bool) { return tasks; }, Date{2026y, October, 7d},
                               {});
    for (int i = 0; i < 19; ++i)
        list->OnEvent(ftxui::Event::ArrowDown);
    list->OnEvent(ftxui::Event::Return);
    auto out = render(list, 80, 12);
    CHECK(contains(out, "Item 19"));
    CHECK_FALSE(contains(out, "Item 01"));
}

TEST_CASE("reopened action popup highlights the entry Enter acts on") {
    std::string edited;
    auto list = single_task_list(buy_milk(), {.on_edit = [&](auto& id) { edited = id; }});
    list->OnEvent(ftxui::Event::Return); // open
    for (int i = 0; i < 3; ++i)
        list->OnEvent(ftxui::Event::ArrowDown);
    list->OnEvent(ftxui::Event::Return); // Cancel
    list->OnEvent(ftxui::Event::Return); // open again
    const auto inverted = "\x1b[7m";
    CHECK_FALSE(contains(line_with(render_styled(list), "Cancel"), inverted));
    list->OnEvent(ftxui::Event::ArrowDown);
    CHECK(contains(line_with(render_styled(list), "Edit"), inverted));
    list->OnEvent(ftxui::Event::Return);
    CHECK(edited == "id-1");
}

TEST_CASE("action popup wraps around at the top and bottom") {
    std::string toggled;
    auto list = single_task_list(buy_milk(), {.on_toggle_done = [&](auto& id) { toggled = id; }});
    const auto inverted = "\x1b[7m";
    list->OnEvent(ftxui::Event::Return);  // open on "Toggle done"
    list->OnEvent(ftxui::Event::ArrowUp); // wraps to "Cancel"
    CHECK(contains(line_with(render_styled(list), "Cancel"), inverted));
    list->OnEvent(ftxui::Event::ArrowDown); // wraps back to "Toggle done"
    CHECK(contains(line_with(render_styled(list), "Toggle done"), inverted));
    list->OnEvent(ftxui::Event::ArrowUp);
    list->OnEvent(ftxui::Event::Return); // Cancel: closes without an action
    CHECK(toggled.empty());
    CHECK_FALSE(contains(render(list), "Cancel"));
}

TEST_CASE("notes with Windows line endings and tabs render cleanly") {
    auto t = make_task("Pay taxes");
    t.notes = "Receipts\r\n\r\nDeadline\tper letter\r\n";
    auto out = render(single_task_list(t, {}));
    CHECK(contains(out, "Deadline per letter"));
    CHECK_FALSE(contains(out, "\t"));
    // The screen itself separates rows with "\r\n"; inside a row there must be none.
    auto line = line_with(out, "Receipts");
    if (line.ends_with('\r'))
        line.pop_back();
    CHECK_FALSE(contains(line, "\r"));
    CHECK(line.ends_with("│"));
    // The blank Windows line ("\r" only) is skipped like any blank line.
    const auto receipts = out.find("Receipts");
    const auto deadline = out.find("Deadline");
    REQUIRE(deadline != std::string::npos);
    CHECK(std::count(out.begin() + static_cast<std::ptrdiff_t>(receipts),
                     out.begin() + static_cast<std::ptrdiff_t>(deadline), '\n') == 1);
}

TEST_CASE("show done reloads the list with done tasks") {
    std::vector<bool> requested;
    auto list = make_task_list(
        "All tasks",
        [&](bool include_done) {
            requested.push_back(include_done);
            auto open = make_task("Open task");
            auto done = make_task("Done task");
            done.done = true;
            return include_done ? std::vector<Task>{open, done} : std::vector<Task>{open};
        },
        Date{2026y, October, 7d}, {});
    CHECK_FALSE(contains(render(list), "Done task"));
    list->OnEvent(ftxui::Event::ArrowUp);    // rows -> toolbar (Back)
    list->OnEvent(ftxui::Event::ArrowRight); // -> Show done
    list->OnEvent(ftxui::Event::Return);
    auto out = render(list);
    CHECK(contains(out, "Show done: on"));
    CHECK(contains(out, "Done task"));
    CHECK(requested.back());
    list->OnEvent(ftxui::Event::Return);
    CHECK(contains(render(list), "Show done: off"));
    CHECK_FALSE(requested.back());
}
