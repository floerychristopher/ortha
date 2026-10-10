#pragma once

#include "core/routine_repository.hpp"

#include <ftxui/component/component.hpp>

#include <functional>
#include <string>
#include <vector>

namespace ortha {

struct RoutinesCallbacks {
    std::function<void()> on_back{}; // Esc / Back
    std::function<void()> on_new_routine{};
    std::function<void()> on_month{}; // the month grid
    std::function<void(const std::string& routine_id)> on_add_habit{};
    std::function<void(const std::string& routine_id)> on_edit_routine{};
    std::function<void(const std::string& routine_id)> on_delete_routine{};
    std::function<void(const std::string& goal_id)> on_history{};
    std::function<void(const std::string& goal_id)> on_edit_habit{};
    std::function<void(const std::string& goal_id)> on_delete_habit{};
    std::function<void(const std::string& goal_id, int delta)> on_move{}; // -1 up, +1 down
};

// All routines (by start time) with their habits below them: days, the last seven days and
// the streak. Enter on a routine opens Add habit / Edit / Delete, Enter on a habit opens
// History / Edit / Move up / Move down / Delete; deleting asks first.
ftxui::Component make_routines_view(std::function<std::vector<RoutineSummary>()> load,
                                    RoutinesCallbacks callbacks);

} // namespace ortha
