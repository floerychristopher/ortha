#pragma once

#include "core/task.hpp"

#include <ftxui/component/component.hpp>

#include <functional>
#include <string>
#include <vector>

namespace ortha {

struct TaskListCallbacks {
    std::function<void()> on_back{}; // Esc
    std::function<void(const std::string& id)> on_edit{};
    std::function<void(const std::string& id)> on_toggle_done{};
    std::function<void(const std::string& id)> on_delete{};
};

struct TaskViewOptions {
    std::string heading;
    std::function<std::vector<Task>()> load; // called whenever the rows need (re)loading
    Date today;                              // for coloring overdue / due-today dates
    TaskListCallbacks callbacks;
    std::string empty_text = "nothing here.";
    bool fill_height = true;              // rows take all remaining height (scrollable list)
    bool show_notes = false;              // each task's notes below its row
    std::function<void()> on_changed{};   // after a task was completed, reopened or deleted
};

// A window with selectable task rows (optionally with their notes below them).
// Enter on a row opens the actions popup
// (Toggle done / Edit / Delete, with confirmation). Optional header and footer
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
    const Task& selected_task() const;
    void on_action();
    void on_confirm();
    void close_popups();

    TaskViewOptions options_;
    std::vector<Task> tasks_;
    int selected_row_ = 0;
    int focus_area_ = static_cast<int>(Area::Rows);
    ftxui::Component header_;
    ftxui::Component rows_;
    ftxui::Component footer_;

    bool show_actions_ = false;
    std::vector<std::string> action_entries_{"Toggle done", "Edit", "Delete", "Cancel"};
    int action_selected_ = 0;
    bool show_confirm_ = false;
    std::vector<std::string> confirm_entries_{"Delete", "Cancel"};
    int confirm_selected_ = 0;
};

} // namespace ortha
