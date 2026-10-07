#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/screen/screen.hpp>

#include <regex>
#include <sstream>
#include <string>

namespace ortha::test {

// Renders a component once into an off-screen buffer and returns the screen including
// its escape sequences, e.g. "\x1b[7m" for inverted text. Rows end with "\r\n".
inline std::string render_styled(const ftxui::Component& component, int width = 80,
                                 int height = 24) {
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width),
                                        ftxui::Dimension::Fixed(height));
    ftxui::Render(screen, component->Render());
    return screen.ToString();
}

// Like render_styled(), but only the visible text (color/style escape sequences removed).
inline std::string render(const ftxui::Component& component, int width = 80, int height = 24) {
    return std::regex_replace(render_styled(component, width, height),
                              std::regex("\x1b\\[[0-9;?]*[A-Za-z]"), "");
}

inline bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

// The first rendered line that contains `needle` (empty if none).
inline std::string line_with(const std::string& out, const std::string& needle) {
    std::istringstream lines(out);
    for (std::string line; std::getline(lines, line);)
        if (contains(line, needle))
            return line;
    return {};
}

} // namespace ortha::test
