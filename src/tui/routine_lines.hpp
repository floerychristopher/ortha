#pragma once

#include "core/routine_repository.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ortha {

// One line of a routines list (Today, Routines view): a heading or one of its habits.
struct RoutineLine {
    std::size_t routine = 0;
    std::optional<std::size_t> habit; // none = the heading
};

// Every routine's heading, followed by its habits where `show_habits(routine index)` is true.
inline std::vector<RoutineLine> routine_lines(const std::vector<RoutineSummary>& routines,
                                              const std::function<bool(std::size_t)>& show_habits) {
    std::vector<RoutineLine> lines;
    for (std::size_t r = 0; r < routines.size(); ++r) {
        lines.push_back({r, std::nullopt});
        if (show_habits(r))
            for (std::size_t h = 0; h < routines[r].habits.size(); ++h)
                lines.push_back({r, h});
    }
    return lines;
}

// A key that stays the same when lines move: "routine id/goal id" ("routine id/" for a
// heading). Used to keep the selection on the same line after a reload.
inline std::string line_key(const std::vector<RoutineSummary>& routines, const RoutineLine& line) {
    const auto& r = routines[line.routine];
    return r.routine.id + "/" + (line.habit ? r.habits[*line.habit].goal.id : "");
}

// The index of the line with `key`, if it is still there.
inline std::optional<std::size_t> find_line(const std::vector<RoutineSummary>& routines,
                                            const std::vector<RoutineLine>& lines,
                                            const std::string& key) {
    for (std::size_t i = 0; i < lines.size(); ++i)
        if (line_key(routines, lines[i]) == key)
            return i;
    return std::nullopt;
}

} // namespace ortha
