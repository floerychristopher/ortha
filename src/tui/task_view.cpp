#include "tui/task_view.hpp"

#include "tui/popup.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>

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

Element task_row(const Task& task, Date today, bool focused, bool selected) {
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
    auto row = hbox(std::move(parts));
    if (focused)
        row |= inverted;
    // `focus` tells the scrolling frame which row to keep visible. Use the selected row,
    // not the keyboard focus, so the list does not jump while a popup has the focus.
    if (selected)
        row |= focus;
    return row;
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
        if (tasks_.empty())
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
        focus_area_ == static_cast<int>(Area::Rows) && !tasks_.empty()) {
        action_entries_[0] = selected_task().done ? "Reopen" : "Toggle done";
        action_selected_ = 0;
        show_actions_ = true;
        return true;
    }
    const int before = focus_area_;
    const bool handled = ComponentBase::OnEvent(event);
    // Entering the rows from below lands on the last row, from above on the first one.
    if (focus_area_ == static_cast<int>(Area::Rows) && before != focus_area_ && !tasks_.empty())
        selected_row_ = before == static_cast<int>(Area::Footer)
                            ? static_cast<int>(tasks_.size()) - 1
                            : 0;
    return handled;
}

void TaskView::reload() {
    tasks_ = options_.load();
    rows_->DetachAllChildren();
    for (std::size_t i = 0; i < tasks_.size(); ++i) {
        MenuEntryOption option;
        option.label = tasks_[i].title;
        option.transform = [this, i](const EntryState& state) {
            return task_row(tasks_[i], options_.today, state.focused,
                            static_cast<int>(i) == selected_row_);
        };
        rows_->Add(MenuEntry(option));
    }
    selected_row_ = std::clamp(selected_row_, 0, std::max(0, static_cast<int>(tasks_.size()) - 1));
    if (tasks_.empty() && focus_area_ == static_cast<int>(Area::Rows))
        focus(footer_->ChildCount() > 0 ? Area::Footer : Area::Header);
}

const Task& TaskView::selected_task() const {
    static const Task none;
    return tasks_.empty() ? none : tasks_[static_cast<std::size_t>(selected_row_)];
}

void TaskView::close_popups() { show_actions_ = show_confirm_ = false; }

void TaskView::on_action() {
    show_actions_ = false;
    const auto id = selected_task().id; // copy: reload() replaces tasks_
    const auto& cb = options_.callbacks;
    switch (action_selected_) {
    case 0:
        if (cb.on_toggle_done)
            cb.on_toggle_done(id);
        reload();
        if (options_.on_changed)
            options_.on_changed();
        break;
    case 1:
        if (cb.on_edit)
            cb.on_edit(id);
        break;
    case 2:
        confirm_selected_ = 0;
        show_confirm_ = true;
        break;
    default:
        break;
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
