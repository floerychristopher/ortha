#include "tui/task_form_state.hpp"

namespace ortha {

namespace {

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::optional<int> parse_interval(const std::string& text) {
    const auto s = trim(text);
    if (s.empty() || s.size() > 3 || s.find_first_not_of("0123456789") != std::string::npos)
        return std::nullopt;
    const int value = std::stoi(s);
    if (value < 1)
        return std::nullopt;
    return value;
}

} // namespace

TaskFormState form_state_from(const Task& task) {
    TaskFormState s;
    s.title = task.title;
    s.due = task.due ? format_date(*task.due) : "";
    s.tags = join_tags(task.tags);
    s.notes = task.notes;
    s.priority = static_cast<int>(task.priority);
    s.subtasks = task.subtasks;
    if (task.recurrence) {
        const auto& rule = *task.recurrence;
        s.repeat = static_cast<int>(rule.freq) + 1;
        s.interval = std::to_string(rule.interval);
        for (auto wd : rule.weekdays)
            s.weekdays[wd.iso_encoding() - 1] = true;
        s.anchor = rule.anchor == Anchor::Due ? 0 : 1;
    }
    return s;
}

std::variant<Task, std::string> apply_form(const TaskFormState& state, Task base) {
    auto title = trim(state.title);
    if (title.empty())
        return std::string("Title must not be empty");

    std::optional<Date> due;
    if (!trim(state.due).empty()) {
        due = parse_date(state.due);
        if (!due)
            return std::string("Due date must be DD.MM.YYYY");
    }

    std::optional<RecurrenceRule> recurrence;
    if (state.repeat != 0) {
        if (!due)
            return std::string("Repeat needs a due date");
        auto interval = parse_interval(state.interval);
        if (!interval)
            return std::string("Interval must be a number from 1 to 999");

        RecurrenceRule rule;
        rule.freq = static_cast<Frequency>(state.repeat - 1);
        rule.interval = *interval;
        if (rule.freq == Frequency::Weekly) {
            for (unsigned i = 0; i < 7; ++i)
                if (state.weekdays[i])
                    rule.weekdays.push_back(std::chrono::weekday{(i + 1) % 7});
        }
        rule.anchor = state.anchor == 0 ? Anchor::Due : Anchor::Completion;
        recurrence = std::move(rule);
    }

    base.title = std::move(title);
    base.recurrence = std::move(recurrence);
    base.due = due;
    base.tags = parse_tags(state.tags);
    base.notes = state.notes;
    base.priority = static_cast<Priority>(state.priority);
    base.subtasks.clear();
    for (auto sub : state.subtasks) {
        sub.title = trim(sub.title);
        if (!sub.title.empty())
            base.subtasks.push_back(std::move(sub));
    }
    return base;
}

} // namespace ortha
