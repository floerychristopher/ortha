#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/screen/screen.hpp>

#include <regex>
#include <string>

namespace ortha::test {

// Renders a component once into an off-screen buffer and returns the visible text
// (ANSI color/style escape sequences removed).
inline std::string render(const ftxui::Component& component, int width = 80, int height = 24) {
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width),
                                        ftxui::Dimension::Fixed(height));
    ftxui::Render(screen, component->Render());
    return std::regex_replace(screen.ToString(), std::regex("\x1b\\[[0-9;?]*[A-Za-z]"), "");
}

inline bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace ortha::test
