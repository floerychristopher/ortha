#include "core/goal.hpp"

#include <algorithm>

namespace ortha {

namespace ch = std::chrono;

namespace {

Date previous_day(Date day) { return Date{ch::sys_days{day} - ch::days{1}}; }

} // namespace

int weekday_index(Date day) {
    return static_cast<int>(ch::weekday{ch::sys_days{day}}.iso_encoding()) - 1;
}

bool is_scheduled(const Goal& goal, Date day) {
    return day >= goal.start && goal.weekdays[static_cast<std::size_t>(weekday_index(day))];
}

GoalStatus next_status(GoalStatus status) {
    switch (status) {
    case GoalStatus::Open:
        return GoalStatus::Done;
    case GoalStatus::Done:
        return GoalStatus::Missed;
    case GoalStatus::Missed:
        return GoalStatus::Open;
    }
    return GoalStatus::Open;
}

int streak(const Goal& goal, Date today, const StatusLookup& status_of) {
    int count = 0;
    if (is_scheduled(goal, today)) {
        const auto status = status_of(today);
        if (status == GoalStatus::Missed)
            return 0;
        if (status == GoalStatus::Done)
            count = 1;
    }
    for (Date day = previous_day(today); day >= goal.start; day = previous_day(day)) {
        if (!is_scheduled(goal, day))
            continue;
        if (status_of(day) != GoalStatus::Done)
            break;
        ++count;
    }
    return count;
}

std::array<std::optional<GoalStatus>, 7> last_week(const Goal& goal, Date today,
                                                   const StatusLookup& status_of) {
    std::array<std::optional<GoalStatus>, 7> week{};
    Date day = today;
    for (int i = 6; i >= 0; --i, day = previous_day(day)) {
        if (is_scheduled(goal, day))
            week[static_cast<std::size_t>(i)] = status_of(day);
    }
    return week;
}

std::vector<Date> scheduled_days(const Goal& goal, Date today) {
    std::vector<Date> days;
    for (Date day = today; day >= goal.start; day = previous_day(day)) {
        if (is_scheduled(goal, day))
            days.push_back(day);
    }
    return days;
}

std::string weekdays_label(const Weekdays& days) {
    if (std::all_of(days.begin(), days.end(), [](bool d) { return d; }))
        return "daily";
    std::string label;
    for (std::size_t i = 0; i < days.size(); ++i) {
        if (!days[i])
            continue;
        if (!label.empty())
            label += ' ';
        label += kWeekdayNames[i];
    }
    return label;
}

std::vector<Date> days_of_month(ch::year_month month) {
    const auto last = ch::year_month_day_last{month.year(), ch::month_day_last{month.month()}};
    std::vector<Date> days;
    for (unsigned d = 1; d <= static_cast<unsigned>(last.day()); ++d)
        days.push_back(month / ch::day{d});
    return days;
}

CellState month_cell(const Goal& goal, Date day, Date today, GoalStatus status) {
    if (!is_scheduled(goal, day))
        return CellState::NotScheduled;
    if (day > today)
        return CellState::Future;
    switch (status) {
    case GoalStatus::Done:
        return CellState::Done;
    case GoalStatus::Missed:
        return CellState::Missed;
    default:
        return CellState::Open;
    }
}

} // namespace ortha
