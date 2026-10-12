#pragma once

#include "core/task.hpp"

#include <ftxui/component/component.hpp>

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ortha {

struct TaskListCallbacks {
    std::function<void()> on_back{}; // Esc
    std::function<void(const std::string& id)> on_edit{};
    std::function<void(const std::string& id)> on_toggle_done{};
    std::function<void(const std::string& id)> on_delete{};
    // Enter on a subtask line: tick it or take the tick back.
    std::function<void(const std::string& task_id, const std::string& subtask_id)>
        on_toggle_subtask{};
};

struct TaskViewOptions {
    std::string heading;
    std::function<std::vector<Task>()> load; // called whenever the rows need (re)loading
    Date today;                              // for coloring overdue / due-today dates
    TaskListCallbacks callbacks;
    std::string empty_text = "nothing here.";
    bool fill_height = true;              // rows take all remaining height (scrollable list)
    bool show_notes = false;              // each task's notes below its row
    bool subtasks_open = false;           // subtasks listed below their task to begin with
    std::function<void()> on_changed{};   // after a task was completed, reopened or deleted
};

// A window with selectable task rows, optionally followed by their subtasks and notes.
// Enter on a task opens the actions popup (Toggle done / Show or Hide subtasks / Edit /
// Delete, with confirmation); Enter on a subtask ticks it. Optional header and footer
// components (toolbar, menu) are shown above and below the rows.
class TaskView : public ftxui::ComponentBase {
  public:
    enum class Area { Header = 0, Rows = 1, Footer = 2 };

    explicit TaskView(TaskViewOptions options);

    void set_header(ftxui::Component header);
    void set_footer(ftxui::Component footer);
    void reload();
    void focus(Area area);

    bool OnEvent(ftxui::Event event) override;

  private:
    // One selectable line: a task, or one of its subtasks.
    struct Line {
        std::size_t task = 0;
        std::optional<std::size_t> subtask;
    };

    const Task& selected_task() const;
    bool subtasks_shown(const Task& task) const;
    std::string line_key(const Line& line) const;
    void on_action();
    void on_confirm();
    void close_popups();

    TaskViewOptions options_;
    std::vector<Task> tasks_;
    std::vector<Line> lines_;
    std::map<std::string, bool> subtasks_open_; // shown/hidden by hand, by task id
    int selected_row_ = 0; // index into lines_
    int focus_area_ = static_cast<int>(Area::Rows);
    ftxui::Component header_;
    ftxui::Component rows_;
    ftxui::Component footer_;

    bool show_actions_ = false;
    std::vector<std::string> action_entries_;
    int action_selected_ = 0;
    bool show_confirm_ = false;
    std::vector<std::string> confirm_entries_{"Delete", "Cancel"};
    int confirm_selected_ = 0;
};

} // namespace ortha
