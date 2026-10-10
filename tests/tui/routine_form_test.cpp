#include "tui/routine_form.hpp"
#include "render.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;
using namespace ortha::test;

namespace {

std::string error_of(const RoutineFormState& state) {
    return std::get<std::string>(apply_routine_form(state, Routine{}));
}

} // namespace

TEST_CASE("apply_routine_form validates name and times") {
    CHECK(error_of({"  ", "8:00", "", ""}) == "Name must not be empty");
    CHECK(error_of({"Sport", "", "", ""}) == "Start must be HH:MM");
    CHECK(error_of({"Sport", "25:00", "", ""}) == "Start must be HH:MM");
    CHECK(error_of({"Sport", "19:30", "9pm", ""}) == "End must be HH:MM");
    CHECK(error_of({"Sport", "19:30", "19:30", ""}) == "End must be after start");
    CHECK(error_of({"Sport", "19:30", "18:00", ""}) == "End must be after start");
}

TEST_CASE("apply_routine_form keeps identity and trims") {
    Routine base;
    base.id = "r1";
    base.created_at = "2026-10-01T00:00:00Z";
    auto r = std::get<Routine>(
        apply_routine_form({" Sport ", " 19:30 ", "21:30", " I move every day. "}, base));
    CHECK(r.id == "r1");
    CHECK(r.created_at == "2026-10-01T00:00:00Z");
    CHECK(r.name == "Sport");
    CHECK(r.start == 1170);
    CHECK(r.end == 1290);
    CHECK(r.identity == "I move every day.");

    auto no_end = std::get<Routine>(apply_routine_form({"Night routine", "23:00", " ", ""}, base));
    CHECK_FALSE(no_end.end);
}

TEST_CASE("routine_form_state_from shows the times") {
    Routine r;
    r.id = "r1";
    r.name = "Sport";
    r.start = 1170;
    r.end = 1290;
    r.identity = "I move every day.";
    auto s = routine_form_state_from(r);
    CHECK(s.name == "Sport");
    CHECK(s.start == "19:30");
    CHECK(s.end == "21:30");
    CHECK(s.identity == "I move every day.");
    auto fresh = routine_form_state_from(Routine{}); // a new routine starts empty
    CHECK(fresh.start.empty());
    CHECK(fresh.end.empty());
}

TEST_CASE("routine form renders, saves on enter and shows errors") {
    std::optional<Routine> saved;
    bool cancelled = false;
    auto form = make_routine_form(
        "New routine", Routine{}, [&](Routine r) { saved = r; }, [&] { cancelled = true; });
    auto out = render(form);
    for (auto text : {"New routine", "Name", "Start", "End", "Identity", "hh:mm",
                      "e.g. i start the day with prayer", "Save", "Cancel"}) {
        INFO(text);
        CHECK(contains(out, text));
    }
    for (char c : std::string("Sport"))
        form->OnEvent(ftxui::Event::Character(c));
    form->OnEvent(ftxui::Event::Return);
    CHECK_FALSE(saved);
    CHECK(contains(render(form), "Start must be HH:MM"));

    form->OnEvent(ftxui::Event::Tab); // Name -> Start
    for (char c : std::string("19:30"))
        form->OnEvent(ftxui::Event::Character(c));
    form->OnEvent(ftxui::Event::Return);
    REQUIRE(saved);
    CHECK(saved->name == "Sport");
    CHECK(saved->start == 1170);

    form->OnEvent(ftxui::Event::Escape);
    CHECK(cancelled);
}

TEST_CASE("a routine may end after midnight") {
    auto r = std::get<Routine>(apply_routine_form({"Night routine", "23:30", "0:30", ""}, Routine{}));
    CHECK(r.start == 23 * 60 + 30);
    CHECK(r.end == 30);
    CHECK(error_of({"Focus", "10:00", "9:00", ""}) == "End must be after start");
    CHECK(error_of({"Early", "3:00", "5:00", ""}) == "End must be after start"); // crosses 4:00
}

TEST_CASE("a routine may end at 4:00, the day change") {
    auto r = std::get<Routine>(apply_routine_form({"Late shift", "22:00", "4:00", ""}, Routine{}));
    CHECK(r.end == 4 * 60);
}
