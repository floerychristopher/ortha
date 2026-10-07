#pragma once

#include <string>
#include <string_view>

namespace ortha {

// Removes leading and trailing whitespace.
inline std::string trim(std::string_view s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
        return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return std::string(s.substr(first, last - first + 1));
}

} // namespace ortha
