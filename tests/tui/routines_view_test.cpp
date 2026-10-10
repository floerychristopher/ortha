#include "tui/routines_view.hpp"
#include "render.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;

namespace {

constexpr auto Done = GoalStatus::Done;
constexpr auto Missed = GoalStatus::Missed;
constexpr auto Open = GoalStatus::Open;
constexpr std::optional<GoalStatus> No = std::nullopt;

GoalSummary habit(std::string id, std::string symbol, std::string title, int streak) {
    GoalSummary h;
    h.goal.id = std::move(id);
    h.goal.symbol = std::move(symbol);
    h.goal.title = std::move(title);
    h.streak = streak;
    h.last_week = {Done, Done, Missed, Done, Done, Done, Open};
    return h;
}

std::vector<RoutineSummary> two_routines() {
    RoutineSummary morning;
    morning.routine.id = "morning";
    morning.routine.name = "Morning routine";
    morning.routine.start = 480;
    morning.streak = 12;
    morning.habits = {habit("water", "💧", "Water", 12), habit("pray", "🙏", "Pray", 5)};

    RoutineSummary sport;
    sport.routine.id = "sport";
    sport.routine.name = "Sport";
    sport.routine.start = 1170;
    sport.routine.end = 1290;
    auto gym = habit("gym", "", "Gym", 3);
    gym.goal.weekdays = {true, false, true, false, true, false, false};
    gym.last_week = {No, Done, No, No, Done, No, Missed};
    sport.habits = {gym};
    return {morning, sport};
}

} // namespace

TEST_CASE("routines view lists routines with their habits") {
    auto out = render(make_routines_view([] { return two_routines(); }, {}), 80, 24);
    for (auto text : {"Routines", "Back", "New routine", "Month"}) {
        INFO(text);
        CHECK(contains(out, text));
    }
    auto morning = line_with(out, "Morning routine");
    CHECK(contains(morning, "8:00"));
    CHECK(contains(morning, "🔥 12"));
    CHECK(contains(out, "  💧 Water"));
    CHECK(contains(line_with(out, "Water"), "daily"));
    CHECK(contains(line_with(out, "Water"), "✓✓✗✓✓✓○"));
    CHECK(contains(line_with(out, "Sport"), "19:30–21:30"));
    CHECK_FALSE(contains(out, "Gym Gym")); // no symbol: only the title, like on Today
    auto gym = line_with(out, "Gym");
    CHECK(contains(gym, "Mo We Fr"));
    CHECK(contains(gym, "·✓··✓·✗"));
    CHECK(contains(gym, "🔥 3"));
    CHECK(out.find("Morning routine") < out.find("Water"));
    CHECK(out.find("Water") < out.find("Pray"));
    CHECK(out.find("Pray") < out.find("Sport"));
}

TEST_CASE("empty routines view") {
    bool back = false;
    auto view = make_routines_view([] { return std::vector<RoutineSummary>{}; },
                                   {.on_back = [&] { back = true; }});
    CHECK(contains(render(view), "no routines yet."));
    view->OnEvent(ftxui::Event::Escape);
    CHECK(back);
}

TEST_CASE("routine popup: add habit, edit, delete with confirmation") {
    std::string added, edited, deleted;
    auto view = make_routines_view(
        [] { return two_routines(); },
        {.on_add_habit = [&](const std::string& id) { added = id; },
         .on_edit_routine = [&](const std::string& id) { edited = id; },
         .on_delete_routine = [&](const std::string& id) { deleted = id; }});
    view->OnEvent(ftxui::Event::Return); // on "Morning routine"
    auto out = render(view);
    for (auto text : {"Add habit", "Edit", "Delete", "Cancel"}) {
        INFO(text);
        CHECK(contains(out, text));
    }
    view->OnEvent(ftxui::Event::Return);
    CHECK(added == "morning");

    view->OnEvent(ftxui::Event::Return);
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::Return);
    CHECK(edited == "morning");

    view->OnEvent(ftxui::Event::Return);
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::Return);
    CHECK(deleted.empty());
    CHECK(contains(render(view), "Delete \"Morning routine\" and its 2 habits?"));
    view->OnEvent(ftxui::Event::Return);
    CHECK(deleted == "morning");
}

TEST_CASE("deleting a routine with one habit says habit") {
    auto view = make_routines_view([] { return two_routines(); }, {});
    for (int i = 0; i < 3; ++i) // Morning -> Water -> Pray -> Sport
        view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::Return);
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::Return);
    CHECK(contains(render(view), "Delete \"Sport\" and its 1 habit?"));
}

TEST_CASE("habit popup: history, move, edit, delete") {
    std::string history, edited, deleted;
    std::vector<std::pair<std::string, int>> moves;
    auto view = make_routines_view(
        [] { return two_routines(); },
        {.on_history = [&](const std::string& id) { history = id; },
         .on_edit_habit = [&](const std::string& id) { edited = id; },
         .on_delete_habit = [&](const std::string& id) { deleted = id; },
         .on_move = [&](const std::string& id, int delta) { moves.emplace_back(id, delta); }});
    view->OnEvent(ftxui::Event::ArrowDown); // Water
    view->OnEvent(ftxui::Event::Return);
    auto out = render(view);
    for (auto text : {"History", "Edit", "Move up", "Move down", "Delete", "Cancel"}) {
        INFO(text);
        CHECK(contains(out, text));
    }
    view->OnEvent(ftxui::Event::Return);
    CHECK(history == "water");

    auto choose = [&](int downs) {
        view->OnEvent(ftxui::Event::Return);
        for (int i = 0; i < downs; ++i)
            view->OnEvent(ftxui::Event::ArrowDown);
        view->OnEvent(ftxui::Event::Return);
    };
    choose(1);
    CHECK(edited == "water");
    choose(2);
    choose(3);
    CHECK(moves == std::vector<std::pair<std::string, int>>{{"water", -1}, {"water", +1}});
    choose(4);
    CHECK(deleted.empty());
    CHECK(contains(render(view), "Delete \"Water\"?"));
    view->OnEvent(ftxui::Event::Return);
    CHECK(deleted == "water");
}

TEST_CASE("routines toolbar and escape") {
    int new_routine = 0, month = 0;
    bool back = false;
    auto view = make_routines_view([] { return two_routines(); },
                                   {.on_back = [&] { back = true; },
                                    .on_new_routine = [&] { ++new_routine; },
                                    .on_month = [&] { ++month; }});
    view->OnEvent(ftxui::Event::Return); // popup on Morning
    view->OnEvent(ftxui::Event::Escape); // closes it first
    CHECK_FALSE(back);
    CHECK_FALSE(contains(render(view), "Add habit"));
    view->OnEvent(ftxui::Event::ArrowUp);    // rows -> toolbar (Back)
    view->OnEvent(ftxui::Event::ArrowRight); // New routine
    view->OnEvent(ftxui::Event::Return);
    CHECK(new_routine == 1);
    view->OnEvent(ftxui::Event::ArrowRight); // Month
    view->OnEvent(ftxui::Event::Return);
    CHECK(month == 1);
    view->OnEvent(ftxui::Event::Escape);
    CHECK(back);
}

TEST_CASE("a moved habit stays selected") {
    auto routines = two_routines();
    auto view = make_routines_view(
        [&] { return routines; },
        {.on_move = [&](const std::string&, int delta) {
            auto& habits = routines[0].habits; // Water, Pray
            if (delta < 0)
                std::swap(habits[0], habits[1]);
        }});
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::ArrowDown); // Pray
    view->OnEvent(ftxui::Event::Return);
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::ArrowDown); // Move up
    view->OnEvent(ftxui::Event::Return);
    CHECK(render(view).find("Pray") < render(view).find("Water"));
    view->OnEvent(ftxui::Event::Return); // the popup opens for the moved habit again
    CHECK(contains(render(view), "╭ Pray"));
}

TEST_CASE("cancel in the routines popups does nothing") {
    int calls = 0;
    auto count = [&](const std::string&) { ++calls; };
    auto view = make_routines_view(
        [] { return two_routines(); },
        {.on_add_habit = count, .on_edit_routine = count, .on_delete_routine = count,
         .on_history = count, .on_edit_habit = count, .on_delete_habit = count,
         .on_move = [&](const std::string&, int) { ++calls; }});
    auto choose = [&](int downs) {
        view->OnEvent(ftxui::Event::Return);
        for (int i = 0; i < downs; ++i)
            view->OnEvent(ftxui::Event::ArrowDown);
        view->OnEvent(ftxui::Event::Return);
    };
    choose(3); // routine popup: Cancel
    choose(2); // routine popup: Delete ...
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::Return); // ... and Cancel in the confirmation
    view->OnEvent(ftxui::Event::ArrowDown); // Water
    choose(5);                              // habit popup: Cancel
    choose(4);                              // habit popup: Delete ...
    view->OnEvent(ftxui::Event::ArrowDown);
    view->OnEvent(ftxui::Event::Return); // ... and Cancel
    CHECK(calls == 0);
    CHECK_FALSE(contains(render(view), "Delete \""));
}
