#pragma once

#include <span>
#include <string_view>

namespace ortha {

// A bible passage for the start screen, e.g. {"Psalm 23,4", "Und ob ich schon …"}.
struct BibleVerse {
    std::string_view reference;
    std::string_view text;
};

// The passages to pick from (Luther 1912), in a fixed order.
std::span<const BibleVerse> bible_verses();

} // namespace ortha
