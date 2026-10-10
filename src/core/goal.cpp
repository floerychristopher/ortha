#include "core/goal.hpp"

#include <algorithm>

namespace ortha {

namespace ch = std::chrono;

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
        return GoalStatus::Skipped;
    case GoalStatus::Skipped:
        return GoalStatus::Open;
    }
    return GoalStatus::Open;
}

GoalStatus status_on(const std::map<Date, GoalStatus>& entries, Date day) {
    const auto it = entries.find(day);
    return it == entries.end() ? GoalStatus::Open : it->second;
}

int chain_length(Date today, Date first, const std::function<DayRating(Date)>& rate) {
    int count = 0;
    bool missed_before = false; // the newer rated day was a miss
    for (Date day = today; day >= first; day = previous_day(day)) {
        switch (rate(day)) {
        case DayRating::Pass:
            break;
        case DayRating::Done:
            ++count;
            missed_before = false;
            break;
        case DayRating::Miss:
            if (missed_before)
                return count; // two misses in a row
            missed_before = true;
            break;
        }
    }
    return count;
}

int streak(const Goal& goal, Date today, const StatusLookup& status_of) {
    return chain_length(today, goal.start, [&](Date day) {
        if (!is_scheduled(goal, day))
            return DayRating::Pass;
        const auto status = status_of(day);
        if (status == GoalStatus::Skipped || (day == today && status == GoalStatus::Open))
            return DayRating::Pass;
        return status == GoalStatus::Done ? DayRating::Done : DayRating::Miss;
    });
}

bool missed_last_time(const Goal& goal, Date today, const StatusLookup& status_of) {
    if (!is_scheduled(goal, today) || status_of(today) != GoalStatus::Open)
        return false;
    for (Date day = previous_day(today); day >= goal.start; day = previous_day(day)) {
        if (!is_scheduled(goal, day))
            continue;
        const auto status = status_of(day);
        if (status == GoalStatus::Skipped)
            continue;
        return status != GoalStatus::Done;
    }
    return false;
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

GoalSummary summarize(const Goal& goal, Date today, const StatusLookup& status_of) {
    GoalSummary s;
    s.goal = goal;
    s.today = status_of(today);
    s.streak = streak(goal, today, status_of);
    s.last_week = last_week(goal, today, status_of);
    s.missed_last_time = missed_last_time(goal, today, status_of);
    return s;
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
    if (status == GoalStatus::Skipped)
        return CellState::Skipped;
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
