#include "tui/error_guard.hpp"
#include "render.hpp"

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>

using namespace ortha;

TEST_CASE("error guard reports exceptions from event handlers") {
    auto failing = ftxui::CatchEvent(ftxui::Renderer([] { return ftxui::text("x"); }),
                                     [](const ftxui::Event&) -> bool {
                                         throw std::runtime_error("disk full");
                                     });
    std::string error;
    auto guarded = guard_errors(failing, [&](const std::string& message) { error = message; });
    CHECK(guarded->OnEvent(ftxui::Event::Return));
    CHECK(error == "disk full");
}

TEST_CASE("error guard passes events through when nothing fails") {
    bool seen = false;
    auto ok = ftxui::CatchEvent(ftxui::Renderer([] { return ftxui::text("x"); }),
                                [&](const ftxui::Event&) {
                                    seen = true;
                                    return true;
                                });
    std::string error;
    auto guarded = guard_errors(ok, [&](const std::string& message) { error = message; });
    CHECK(guarded->OnEvent(ftxui::Event::Return));
    CHECK(seen);
    CHECK(error.empty());
}
