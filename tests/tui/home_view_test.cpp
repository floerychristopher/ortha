#include "tui/home_view.hpp"
#include "../core/test_helpers.hpp"
#include "render.hpp"

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <random>
#include <set>
#include <sstream>
#include <string_view>

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

RoutineSummary routine_summary(std::string id, std::string name, int start, std::optional<int> end,
                               std::vector<GoalSummary> habits, int streak = 0,
                               std::string identity = "") {
    RoutineSummary s;
    s.routine.id = std::move(id);
    s.routine.name = std::move(name);
    s.routine.start = start;
    s.routine.end = end;
    s.routine.identity = std::move(identity);
    for (const auto& h : habits) {
        if (h.today == GoalStatus::Skipped) {
            ++s.today.skipped;
            continue;
        }
        ++s.today.planned;
        s.today.done += h.today == GoalStatus::Done ? 1 : 0;
        s.today.open += h.today == GoalStatus::Open ? 1 : 0;
        s.today.missed += h.today == GoalStatus::Missed ? 1 : 0;
    }
    s.habits = std::move(habits);
    s.streak = streak;
    return s;
}

// From the menu up to the first line of the routines section.
void to_first_routine_line(const ftxui::Component& home) {
    for (int i = 0; i < 20; ++i) // past the tasks, then up to the top of the routines
        home->OnEvent(ftxui::Event::ArrowUp);
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
                   "All tasks (5)", "Routines", "Quit"}) {
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
    CHECK(contains(render(home), "nothing to do."));
    CHECK(contains(render(home), "All tasks (0)"));
}

TEST_CASE("home starts on 'New task' and the menu triggers its actions") {
    int new_count = 0, all_count = 0, routines_count = 0, quit_count = 0;
    auto o = options_with_tasks(3);
    o.actions = {.on_new = [&] { ++new_count; },
                 .on_all = [&] { ++all_count; },
                 .on_routines = [&] { ++routines_count; },
                 .on_quit = [&] { ++quit_count; }};
    auto home = make_home(o);
    home->OnEvent(ftxui::Event::Return);
    CHECK(new_count == 1);
    home->OnEvent(ftxui::Event::ArrowDown);
    home->OnEvent(ftxui::Event::Return);
    CHECK(all_count == 1);
    home->OnEvent(ftxui::Event::ArrowDown);
    home->OnEvent(ftxui::Event::Return);
    CHECK(routines_count == 1);
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
    CHECK(contains(out, "All tasks (2)"));
    CHECK_FALSE(contains(out, "Gym"));
}

// 14:00: Morning (8:00) is done, Focus (10:00–16:00) is on, Sport (19:30–21:30) comes later.
std::vector<RoutineSummary> day_at_two(GoalStatus pray = GoalStatus::Done) {
    return {routine_summary("morning", "Morning routine", 480, std::nullopt,
                            {goal_summary("water", "Water", GoalStatus::Done, 12),
                             goal_summary("pray", "Pray", pray, 12)},
                            12),
            routine_summary("focus", "Focus time", 600, 960,
                            {goal_summary("deep", "Deep work", GoalStatus::Open, 4)}, 4,
                            "I do deep work before anything else."),
            routine_summary("sport", "Sport", 1170, 1290,
                            {goal_summary("gym", "Lift weights", GoalStatus::Open, 2)})};
}

HomeOptions options_with_routines(std::vector<RoutineSummary> routines) {
    auto o = options_with_tasks();
    o.load_routines = [routines] { return routines; };
    o.now_minutes = [] { return 14 * 60; };
    return o;
}

TEST_CASE("today shows routines, the current one expanded") {
    auto out = render(make_home(options_with_routines(day_at_two())), 80, 30);
    auto morning = line_with(out, "Morning routine");
    CHECK(contains(morning, "8:00"));
    CHECK(contains(morning, "2/2 ✓"));
    CHECK(contains(morning, "🔥 12"));
    CHECK_FALSE(contains(out, "Water")); // done: collapsed

    CHECK(contains(line_with(out, "Focus time"), "10:00–16:00"));
    CHECK(contains(out, "I do deep work before anything else."));
    CHECK(contains(line_with(out, "Deep work"), "> [ ]"));
    CHECK(contains(line_with(out, "Deep work"), "🔥 4"));

    auto sport = line_with(out, "Sport");
    CHECK(contains(sport, "19:30–21:30"));
    CHECK(contains(sport, "0/1"));
    CHECK_FALSE(contains(out, "Lift weights")); // later: collapsed
    CHECK(out.find("Morning routine") < out.find("Pay taxes"));
}

TEST_CASE("earlier routines with open habits stay expanded") {
    auto out = render(make_home(options_with_routines(day_at_two(GoalStatus::Open))), 80, 30);
    CHECK(contains(line_with(out, "Morning routine"), "1/2"));
    CHECK(contains(line_with(out, "Water"), "[✓]")); // expanded: done habits are listed too
    CHECK_FALSE(contains(line_with(out, "Water"), ">")); // the marker is on the first open one
    CHECK(contains(line_with(out, "Pray"), "> [ ]"));
    CHECK(contains(line_with(out, "Deep work"), "> [ ]"));
}

TEST_CASE("enter on a heading opens and closes it, on a habit cycles it") {
    std::string cycled;
    auto o = options_with_routines(day_at_two());
    o.on_cycle_habit = [&](const std::string& id) { cycled = id; };
    auto home = make_home(o);
    to_first_routine_line(home);
    home->OnEvent(ftxui::Event::Return); // Morning (done, closed) opens
    CHECK(contains(render(home, 80, 30), "Water"));
    home->OnEvent(ftxui::Event::Return); // and closes again
    CHECK_FALSE(contains(render(home, 80, 30), "Water"));
    home->OnEvent(ftxui::Event::ArrowDown); // Focus time heading
    home->OnEvent(ftxui::Event::ArrowDown); // Deep work
    home->OnEvent(ftxui::Event::Return);
    CHECK(cycled == "deep");
}

TEST_CASE("missed last time and the rest-day heading") {
    auto routines = day_at_two();
    routines[1].habits[0].missed_last_time = true;
    routines[1].habits[0].streak = 0;
    routines[2].habits[0].today = GoalStatus::Skipped;
    routines[2].today = RoutineDay{.planned = 0, .skipped = 1};
    auto out = render(make_home(options_with_routines(routines)), 80, 30);
    CHECK(contains(line_with(out, "Deep work"), "missed last time"));
    auto sport = line_with(out, "Sport");
    CHECK(contains(sport, "·"));
    CHECK_FALSE(contains(sport, "0/0"));
}

TEST_CASE("home without routines has no routines section") {
    auto out = render(make_home(options_with_tasks()));
    CHECK(count(out, "Routines") == 1); // only the menu entry
    CHECK_FALSE(contains(out, "routine"));
}

TEST_CASE("long habit titles keep the streak visible") {
    auto routines = day_at_two();
    routines[1].habits[0].goal.title = std::string(100, 'x');
    routines[1].habits[0].streak = 3;
    CHECK(contains(render(make_home(options_with_routines(routines)), 50, 30), "🔥 3"));
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

TEST_CASE("home does not show task notes") {
    auto o = options_with_tasks();
    o.load_tasks = [] {
        auto tasks = three_tasks();
        tasks[0].notes = "Receipts are in the 2025 folder";
        return tasks;
    };
    CHECK_FALSE(contains(render(make_home(o)), "Receipts"));
}

TEST_CASE("home_height counts the rows the start screen needs") {
    auto o = options_with_tasks();
    int rows = 0; // lines that belong to the window (all others are blank)
    // Inside a container, like in the app, the window keeps its natural height.
    std::istringstream lines(render(ftxui::Container::Vertical({make_home(o)}), 80, 60));
    for (std::string line; std::getline(lines, line);)
        if (line.find_first_not_of(" \r") != std::string::npos)
            ++rows;
    CHECK(home_height(o, 80) == rows);
}

TEST_CASE("pick_fitting_verse only picks verses that fit completely") {
    const std::vector<BibleVerse> verses{
        {"Short 1,1", "A short verse."},
        {"Long 2,2", "A long verse that needs several lines on an eighty column screen, because "
                     "it goes on and on and on, much longer than any line of the start screen "
                     "could ever hold, and then it goes on a little more for good measure."}};
    auto o = options_with_tasks();
    auto with_short = o;
    with_short.verse = verses[0];
    const int just_short = home_height(with_short, 80); // enough for the short verse only

    std::set<std::string_view> picked;
    for (unsigned seed = 0; seed < 30; ++seed) {
        std::mt19937 rng{seed};
        auto v = pick_fitting_verse(o, 80, just_short, verses, rng);
        REQUIRE(v);
        picked.insert(v->reference);
    }
    CHECK(picked == std::set<std::string_view>{"Short 1,1"});

    picked.clear();
    for (unsigned seed = 0; seed < 30; ++seed) {
        std::mt19937 rng{seed};
        picked.insert(pick_fitting_verse(o, 80, 60, verses, rng)->reference);
    }
    CHECK(picked.size() == 2); // with enough room every verse can come up

    std::mt19937 rng{1};
    CHECK_FALSE(pick_fitting_verse(o, 80, home_height(o, 80), verses, rng)); // no room at all
}

TEST_CASE("a picked verse is shown completely") {
    auto o = options_with_tasks();
    std::mt19937 rng{7};
    const int height = home_height(o, 80) + 6;
    o.verse = pick_fitting_verse(o, 80, height, bible_verses(), rng);
    REQUIRE(o.verse);
    auto out = render(ftxui::Container::Vertical({make_home(o)}), 80, height);
    CHECK(contains(out, std::string(o.verse->reference)));
    CHECK(contains(out, "╰")); // the window is not cut off
}

namespace {

// Routines whose habits change when they are rated, like with the real database.
struct LiveDay {
    std::map<std::string, GoalStatus> status{{"a1", GoalStatus::Open},
                                             {"a2", GoalStatus::Open},
                                             {"b1", GoalStatus::Open}};
    int now = 11 * 60;

    std::vector<RoutineSummary> load() const {
        return {routine_summary("alpha", "Alpha", 8 * 60, std::nullopt,
                                {goal_summary("a1", "A one", status.at("a1"), 0),
                                 goal_summary("a2", "A two", status.at("a2"), 0)}),
                routine_summary("beta", "Beta", 10 * 60, std::nullopt,
                                {goal_summary("b1", "B one", status.at("b1"), 0)})};
    }

    HomeOptions options() {
        auto o = options_with_tasks();
        o.load_routines = [this] { return load(); };
        o.now_minutes = [this] { return now; };
        o.on_cycle_habit = [this](const std::string& id) { status[id] = next_status(status[id]); };
        return o;
    }
};

} // namespace

TEST_CASE("the last open habit of a routine can be cycled on") {
    LiveDay day;
    auto home = make_home(day.options());
    to_first_routine_line(home);
    for (int i = 0; i < 4; ++i) // Alpha, A one, A two, Beta, B one
        home->OnEvent(ftxui::Event::ArrowDown);
    home->OnEvent(ftxui::Event::Return); // B one: done, Beta has nothing open any more
    CHECK(day.status["b1"] == GoalStatus::Done);
    home->OnEvent(ftxui::Event::Return); // still on B one: missed
    CHECK(day.status["b1"] == GoalStatus::Missed);
    CHECK(day.status["a1"] == GoalStatus::Open);
}

TEST_CASE("the current routine follows the clock") {
    LiveDay day;
    day.now = 9 * 60; // only Alpha has begun
    auto home = make_home(day.options());
    CHECK_FALSE(contains(render(home, 80, 30), "B one"));
    day.now = 11 * 60;
    to_first_routine_line(home);
    home->OnEvent(ftxui::Event::ArrowDown); // A one
    home->OnEvent(ftxui::Event::Return);    // any change reloads the routines
    CHECK(contains(render(home, 80, 30), "B one"));
}

TEST_CASE("the selected routine heading is highlighted") {
    auto home = make_home(options_with_routines(day_at_two()));
    to_first_routine_line(home);
    CHECK(contains(line_with(render_styled(home, 80, 30), "Morning routine"), "\x1b[7m"));
}

TEST_CASE("a routine with ten habits keeps its tick") {
    std::vector<GoalSummary> habits;
    for (int i = 0; i < 10; ++i)
        habits.push_back(goal_summary("h" + std::to_string(i), "Habit " + std::to_string(i),
                                      GoalStatus::Done, 0));
    auto out = render(make_home(options_with_routines({routine_summary(
                          "big", "Big routine", 480, std::nullopt, habits, 3)})),
                      80, 40);
    CHECK(contains(line_with(out, "Big routine"), "10/10 ✓"));
}

TEST_CASE("a routine that has not begun can be opened") {
    auto home = make_home(options_with_routines(day_at_two()));
    to_first_routine_line(home);
    for (int i = 0; i < 3; ++i) // Morning -> Focus -> Deep work -> Sport
        home->OnEvent(ftxui::Event::ArrowDown);
    home->OnEvent(ftxui::Event::Return);
    CHECK(contains(render(home, 80, 30), "Lift weights"));
}

TEST_CASE("opened and closed routines stay so when Today is built again") {
    auto o = options_with_routines(day_at_two());
    auto shared = std::make_shared<std::map<std::string, bool>>();
    o.expanded = [shared] { return shared; };
    {
        auto home = make_home(o);
        to_first_routine_line(home);
        home->OnEvent(ftxui::Event::ArrowDown); // Focus time (open on its own)
        home->OnEvent(ftxui::Event::Return);    // closed by hand
    }
    auto out = render(make_home(o), 80, 30);
    CHECK_FALSE(contains(out, "Deep work"));
}

TEST_CASE("the routine on now and routines with open habits are coloured") {
    auto out = render_styled(make_home(options_with_routines(day_at_two(GoalStatus::Open))), 80, 30);
    auto focus = line_with(out, "Focus time");
    CHECK(contains(focus, "▶"));
    CHECK(contains(focus, "\x1b[36m")); // cyan: 14:00 is inside 10:00–16:00
    CHECK(contains(line_with(out, "Morning routine"), "\x1b[33m")); // yellow: begun, Pray open
    auto sport = line_with(out, "Sport");
    CHECK_FALSE(contains(sport, "\x1b[36m")); // later: plain
    CHECK_FALSE(contains(sport, "\x1b[33m"));
    CHECK_FALSE(contains(sport, "▶"));
}

TEST_CASE("coming up from the tasks lands on the last routine line") {
    auto home = make_home(options_with_routines(day_at_two()));
    for (int i = 0; i < 4; ++i) // menu -> 3 task rows -> routines
        home->OnEvent(ftxui::Event::ArrowUp);
    home->OnEvent(ftxui::Event::Return); // the last line is the Sport heading: it opens
    auto out = render(home, 80, 30);
    CHECK(contains(out, "Lift weights"));
    CHECK_FALSE(contains(out, "Water"));
}

TEST_CASE("a click on a routine selects that line, not the last one") {
    auto home = make_home(options_with_routines(day_at_two()));
    auto out = render(home, 80, 30);
    int row = 0;
    std::istringstream lines(out);
    for (std::string line; std::getline(lines, line); ++row)
        if (contains(line, "Morning routine"))
            break;
    ftxui::Mouse click;
    click.button = ftxui::Mouse::Left;
    click.motion = ftxui::Mouse::Pressed;
    click.x = 5;
    click.y = row;
    home->OnEvent(ftxui::Event::Mouse("", click));
    home->OnEvent(ftxui::Event::Return); // opens the clicked routine
    out = render(home, 80, 30);
    CHECK(contains(out, "Water"));
    CHECK_FALSE(contains(out, "Lift weights"));
}

TEST_CASE("open/close choices end with the routine day") {
    auto choices = std::make_shared<std::map<std::string, bool>>();
    auto o = options_with_routines(day_at_two());
    o.expanded = [&] { return choices; };
    auto home = make_home(o);
    to_first_routine_line(home);
    home->OnEvent(ftxui::Event::ArrowDown); // Focus time: open on its own
    home->OnEvent(ftxui::Event::Return);    // closed by hand
    CHECK_FALSE(contains(render(home, 80, 30), "Deep work"));
    choices = std::make_shared<std::map<std::string, bool>>(); // 4:00: a new routine day
    home->OnEvent(ftxui::Event::ArrowUp); // Morning routine
    home->OnEvent(ftxui::Event::Return);  // any reload picks up the new day's choices
    CHECK(contains(render(home, 80, 30), "Deep work")); // Focus opens on its own again
}

TEST_CASE("today keeps subtasks closed until shown") {
    auto o = options_with_tasks();
    std::vector<std::string> ticked;
    o.load_tasks = [] {
        auto tasks = three_tasks();
        tasks[0].subtasks = {{"s1", "Collect receipts", false}, {"s2", "Fill in form", false}};
        return tasks;
    };
    o.task_callbacks.on_toggle_subtask = [&](const std::string&, const std::string& sub) {
        ticked.push_back(sub);
    };
    auto home = make_home(o);
    CHECK_FALSE(contains(render(home, 80, 30), "Collect receipts"));
    for (int i = 0; i < 3; ++i) // menu -> last task -> ... -> Pay taxes
        home->OnEvent(ftxui::Event::ArrowUp);
    home->OnEvent(ftxui::Event::Return);    // popup
    home->OnEvent(ftxui::Event::ArrowDown); // Show subtasks
    home->OnEvent(ftxui::Event::Return);
    CHECK(contains(render(home, 80, 30), "Collect receipts"));
    home->OnEvent(ftxui::Event::ArrowDown); // Collect receipts
    home->OnEvent(ftxui::Event::Return);
    CHECK(ticked == std::vector<std::string>{"s1"});
}
