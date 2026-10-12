#include "tui/task_view.hpp"

#include "tui/popup.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <sstream>

namespace ortha {

using namespace ftxui;

namespace {

const char* priority_marker(Priority p) {
    switch (p) {
    case Priority::Low:
        return "!  ";
    case Priority::Medium:
        return "!! ";
    case Priority::High:
        return "!!!";
    default:
        return "   ";
    }
}

// The non-empty lines of the notes, indented to the title and wrapped. Carriage returns
// (Windows line endings) are dropped, so "\r"-only lines count as blank, and tabs become
// spaces, so the words around them stay apart.
Element notes_block(const std::string& notes) {
    Elements lines;
    std::istringstream stream(notes);
    for (std::string line; std::getline(stream, line);) {
        std::erase(line, '\r');
        std::replace(line.begin(), line.end(), '\t', ' ');
        if (line.find_first_not_of(' ') == std::string::npos)
            continue;
        lines.push_back(hbox({text("        "), paragraph(line) | dim | flex}));
    }
    return vbox(std::move(lines));
}

Element task_row(const Task& task, Date today) {
    Elements parts = {
        text(task.done ? "[x] " : "[ ] "),
        text(priority_marker(task.priority)) | color(Color::Red),
        text(" "),
        text(task.title) | (task.done ? dim : nothing) | flex_shrink, // long titles get cut
        text(task.recurrence ? " ↻" : "") | color(Color::Blue),
        filler(),
    };
    if (!task.subtasks.empty()) {
        const auto done = std::count_if(task.subtasks.begin(), task.subtasks.end(),
                                        [](const Subtask& s) { return s.done; });
        parts.push_back(text(" " + std::to_string(done) + "/" +
                             std::to_string(task.subtasks.size())) |
                        dim);
    }
    for (const auto& tag : task.tags)
        parts.push_back(text(" #" + tag) | color(Color::Cyan));
    if (task.due) {
        auto date = text("  " + due_label(*task.due, today));
        if (!task.done && *task.due < today)
            date |= color(Color::Red);
        else if (!task.done && *task.due == today)
            date |= color(Color::Yellow);
        parts.push_back(date);
    }
    return hbox(std::move(parts));
}

// A subtask, indented to the task's title.
Element subtask_row(const Subtask& subtask) {
    return hbox({text("        " + std::string(subtask.done ? "[x] " : "[ ] ")),
                 text(subtask.title) | (subtask.done ? dim : nothing) | flex_shrink});
}

} // namespace

TaskView::TaskView(TaskViewOptions options) : options_(std::move(options)) {
    header_ = Container::Vertical({});
    rows_ = Container::Vertical({}, &selected_row_);
    footer_ = Container::Vertical({});

    auto body = Renderer(Container::Vertical({header_, rows_, footer_}, &focus_area_), [this] {
        Elements parts;
        if (header_->ChildCount() > 0) {
            parts.push_back(header_->Render());
            parts.push_back(separator());
        }
        if (lines_.empty())
            parts.push_back(text(options_.empty_text) | dim);
        else if (options_.fill_height)
            parts.push_back(rows_->Render() | vscroll_indicator | yframe | flex);
        else
            parts.push_back(rows_->Render());
        if (footer_->ChildCount() > 0) {
            parts.push_back(separator());
            parts.push_back(footer_->Render());
        }
        return window(text(" " + options_.heading + " ") | bold, vbox(std::move(parts)));
    });

    auto actions = popup_menu(&action_entries_, &action_selected_, [this] { on_action(); });
    auto confirm = popup_menu(&confirm_entries_, &confirm_selected_, [this] { on_confirm(); });
    auto actions_popup = Renderer(actions, [this, actions] {
        return window(text(" " + selected_task().title + " "), actions->Render());
    });
    auto confirm_popup = Renderer(confirm, [this, confirm] {
        return window(text(" Delete \"" + selected_task().title + "\"? "), confirm->Render());
    });
    Add(Modal(Modal(body, actions_popup, &show_actions_), confirm_popup, &show_confirm_));

    reload();
}

void TaskView::set_header(Component header) { header_->Add(std::move(header)); }

void TaskView::set_footer(Component footer) { footer_->Add(std::move(footer)); }

void TaskView::focus(Area area) { focus_area_ = static_cast<int>(area); }

bool TaskView::OnEvent(Event event) {
    if (event == Event::Escape) {
        if (show_actions_ || show_confirm_)
            close_popups();
        else if (options_.callbacks.on_back)
            options_.callbacks.on_back();
        return true;
    }
    if (event == Event::Return && !show_actions_ && !show_confirm_ &&
        focus_area_ == static_cast<int>(Area::Rows) && !lines_.empty()) {
        const auto& line = lines_[static_cast<std::size_t>(selected_row_)];
        const auto& task = selected_task();
        if (line.subtask) { // a subtask: tick it right away
            if (options_.callbacks.on_toggle_subtask)
                options_.callbacks.on_toggle_subtask(task.id, task.subtasks[*line.subtask].id);
            reload();
            return true;
        }
        action_entries_ = {task.done ? "Reopen" : "Toggle done"};
        if (!task.subtasks.empty())
            action_entries_.push_back(subtasks_shown(task) ? "Hide subtasks" : "Show subtasks");
        action_entries_.insert(action_entries_.end(), {"Edit", "Delete", "Cancel"});
        action_selected_ = 0;
        show_actions_ = true;
        return true;
    }
    const int before = focus_area_;
    const bool handled = ComponentBase::OnEvent(event);
    // Entering the rows from below lands on the last row, from above on the first one.
    // Only for the keyboard: a click selects exactly the line that was clicked.
    const bool keyboard = !event.is_mouse();
    if (keyboard && focus_area_ == static_cast<int>(Area::Rows) && before != focus_area_ &&
        !lines_.empty())
        selected_row_ = before == static_cast<int>(Area::Footer)
                            ? static_cast<int>(lines_.size()) - 1
                            : 0;
    // The same for the header (e.g. Today's routines): from below, its last line.
    if (keyboard && focus_area_ == static_cast<int>(Area::Header) && before > focus_area_)
        header_->OnEvent(Event::End);
    return handled;
}

bool TaskView::subtasks_shown(const Task& task) const {
    const auto it = subtasks_open_.find(task.id);
    return it == subtasks_open_.end() ? options_.subtasks_open : it->second;
}

std::string TaskView::line_key(const Line& line) const {
    const auto& task = tasks_[line.task];
    return task.id + "/" + (line.subtask ? task.subtasks[*line.subtask].id : "");
}

void TaskView::reload() {
    // Keep the selection on the same task or subtask, wherever it moved.
    const auto before = lines_.empty()
                            ? std::string()
                            : line_key(lines_[static_cast<std::size_t>(selected_row_)]);
    tasks_ = options_.load();
    lines_.clear();
    for (std::size_t t = 0; t < tasks_.size(); ++t) {
        lines_.push_back({t, std::nullopt});
        if (subtasks_shown(tasks_[t]))
            for (std::size_t s = 0; s < tasks_[t].subtasks.size(); ++s)
                lines_.push_back({t, s});
    }
    rows_->DetachAllChildren();
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        MenuEntryOption option;
        option.transform = [this, i](const EntryState& state) {
            const auto& line = lines_[i];
            const auto& task = tasks_[line.task];
            Element row = line.subtask ? subtask_row(task.subtasks[*line.subtask])
                                       : task_row(task, options_.today);
            if (state.focused)
                row |= inverted;
            // The notes follow the task's last line (its last subtask, if they are shown).
            const bool last_of_task = i + 1 == lines_.size() || lines_[i + 1].task != line.task;
            if (options_.show_notes && last_of_task && !task.notes.empty())
                row = vbox({row, notes_block(task.notes)});
            // `focus` tells the scrolling frame which row to keep visible. Use the selected
            // row, not the keyboard focus, so the list does not jump while a popup is open.
            if (static_cast<int>(i) == selected_row_)
                row |= ftxui::focus; // the decorator, not TaskView::focus()
            return row;
        };
        rows_->Add(MenuEntry(option));
    }
    for (std::size_t i = 0; i < lines_.size(); ++i)
        if (!before.empty() && line_key(lines_[i]) == before)
            selected_row_ = static_cast<int>(i);
    selected_row_ = std::clamp(selected_row_, 0, std::max(0, static_cast<int>(lines_.size()) - 1));
    if (lines_.empty() && focus_area_ == static_cast<int>(Area::Rows))
        focus(footer_->ChildCount() > 0 ? Area::Footer : Area::Header);
}

const Task& TaskView::selected_task() const {
    static const Task none;
    return lines_.empty() ? none : tasks_[lines_[static_cast<std::size_t>(selected_row_)].task];
}

void TaskView::close_popups() { show_actions_ = show_confirm_ = false; }

void TaskView::on_action() {
    show_actions_ = false;
    const auto id = selected_task().id; // copy: reload() replaces tasks_
    const auto& cb = options_.callbacks;
    const auto& action = action_entries_[static_cast<std::size_t>(action_selected_)];
    if (action == "Toggle done" || action == "Reopen") {
        if (cb.on_toggle_done)
            cb.on_toggle_done(id);
        reload();
        if (options_.on_changed)
            options_.on_changed();
    } else if (action == "Show subtasks" || action == "Hide subtasks") {
        subtasks_open_[id] = action == "Show subtasks";
        reload();
    } else if (action == "Edit") {
        if (cb.on_edit)
            cb.on_edit(id);
    } else if (action == "Delete") {
        confirm_selected_ = 0;
        show_confirm_ = true;
    }
}

void TaskView::on_confirm() {
    show_confirm_ = false;
    if (confirm_selected_ != 0)
        return;
    const auto id = selected_task().id;
    if (options_.callbacks.on_delete)
        options_.callbacks.on_delete(id);
    reload();
    if (options_.on_changed)
        options_.on_changed();
}

} // namespace ortha
