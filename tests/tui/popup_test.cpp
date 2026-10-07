#include "tui/popup.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;

TEST_CASE("popup menu wraps around with j and k too") {
    std::vector<std::string> entries{"A", "B", "C"};
    int selected = 0;
    auto menu = popup_menu(&entries, &selected, [] {});
    menu->OnEvent(ftxui::Event::Character('k'));
    CHECK(selected == 2);
    menu->OnEvent(ftxui::Event::Character('j'));
    CHECK(selected == 0);
}

TEST_CASE("popup menu without entries keeps a valid selection") {
    std::vector<std::string> entries;
    int selected = 0;
    auto menu = popup_menu(&entries, &selected, [] {});
    menu->OnEvent(ftxui::Event::ArrowUp);
    CHECK(selected == 0);
    menu->OnEvent(ftxui::Event::ArrowDown);
    CHECK(selected == 0);
}
