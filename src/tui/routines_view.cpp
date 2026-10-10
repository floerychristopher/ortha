#include "tui/routines_view.hpp"

#include "tui/goal_widgets.hpp"
#include "tui/popup.hpp"
#include "tui/routine_lines.hpp"

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <format>
#include <optional>

namespace ortha {

using namespace ftxui;

namespace {

Element week_strip(const std::array<std::optional<GoalStatus>, 7>& week) {
    Elements days;
    for (const auto& day : week)
        days.push_back(status_mark(day));
    return hbox(std::move(days));
}

Element routine_row(const RoutineSummary& r) {
    return hbox({
        text(r.routine.name) | bold | flex_shrink,
        text("  " + time_label(r.routine)) | dim,
        filler(),
        streak_label(r.streak, 8),
    });
}

Element habit_row(const GoalSummary& h) {
    return hbox({
        text("  " + habit_label(h.goal)) | flex_shrink,
        filler(),
        text("  " + weekdays_label(h.goal.weekdays)) | size(WIDTH, EQUAL, 22),
        week_strip(h.last_week),
        streak_label(h.streak, 8),
    });
}

class RoutinesView : public ComponentBase {
  public:
    RoutinesView(std::function<std::vector<RoutineSummary>()> load, RoutinesCallbacks callbacks)
        : load_(std::move(load)), callbacks_(std::move(callbacks)) {
        auto back = Button("Back", [this] { go_back(); }, ButtonOption::Ascii());
        auto add = Button("New routine", [this] { call(callbacks_.on_new_routine); },
                          ButtonOption::Ascii());
        auto month = Button("Month", [this] { call(callbacks_.on_month); }, ButtonOption::Ascii());
        toolbar_ = Container::Horizontal({back, add, month});
        rows_ = Container::Vertical({}, &selected_row_);

        auto body = Renderer(Container::Vertical({toolbar_, rows_}, &focus_area_), [this] {
            Element list = lines_.empty()
                               ? text("no routines yet.") | dim
                               : rows_->Render() | vscroll_indicator | yframe | flex;
            return window(text(" Routines ") | bold, vbox({toolbar_->Render(), separator(), list}));
        });
        auto routine_menu = popup_menu(&routine_entries_, &routine_selected_,
                                       [this] { on_routine_action(); });
        auto habit_menu =
            popup_menu(&habit_entries_, &habit_selected_, [this] { on_habit_action(); });
        auto confirm = popup_menu(&confirm_entries_, &confirm_selected_, [this] { on_confirm(); });
        auto routine_popup = Renderer(routine_menu, [this, routine_menu] {
            return window(text(" " + routine().routine.name + " "), routine_menu->Render());
        });
        auto habit_popup = Renderer(habit_menu, [this, habit_menu] {
            return window(text(" " + habit()->goal.title + " "), habit_menu->Render());
        });
        auto confirm_popup = Renderer(confirm, [this, confirm] {
            return window(text(" " + confirm_question() + " "), confirm->Render());
        });
        Add(Modal(Modal(Modal(body, routine_popup, &show_routine_actions_), habit_popup,
                        &show_habit_actions_),
                  confirm_popup, &show_confirm_));

        reload();
        focus_area_ = lines_.empty() ? 0 : 1;
    }

    bool OnEvent(Event event) override {
        const bool popup_open = show_routine_actions_ || show_habit_actions_ || show_confirm_;
        if (event == Event::Escape) {
            if (popup_open)
                show_routine_actions_ = show_habit_actions_ = show_confirm_ = false;
            else
                go_back();
            return true;
        }
        if (event == Event::Return && !popup_open && focus_area_ == 1 && !lines_.empty()) {
            if (habit()) {
                habit_selected_ = 0;
                show_habit_actions_ = true;
            } else {
                routine_selected_ = 0;
                show_routine_actions_ = true;
            }
            return true;
        }
        return ComponentBase::OnEvent(event);
    }

  private:
    static void call(const std::function<void()>& fn) {
        if (fn)
            fn();
    }

    void go_back() { call(callbacks_.on_back); }

    const RoutineLine& line() const { return lines_[static_cast<std::size_t>(selected_row_)]; }
    const RoutineSummary& routine() const {
        static const RoutineSummary none{};
        return lines_.empty() ? none : routines_[line().routine];
    }
    const GoalSummary* habit() const {
        if (lines_.empty() || !line().habit)
            return nullptr;
        return &routine().habits[*line().habit];
    }

    std::string confirm_question() const {
        if (const auto* h = habit())
            return "Delete \"" + h->goal.title + "\"?";
        const auto& r = routine();
        const auto n = r.habits.size();
        return std::format("Delete \"{}\" and its {} {}?", r.routine.name, n,
                           n == 1 ? "habit" : "habits");
    }

    void on_routine_action() {
        show_routine_actions_ = false;
        const auto id = routine().routine.id;
        switch (routine_selected_) {
        case 0:
            if (callbacks_.on_add_habit)
                callbacks_.on_add_habit(id);
            break;
        case 1:
            if (callbacks_.on_edit_routine)
                callbacks_.on_edit_routine(id);
            break;
        case 2:
            confirm_selected_ = 0;
            show_confirm_ = true;
            break;
        default:
            break;
        }
    }

    void on_habit_action() {
        show_habit_actions_ = false;
        const auto id = habit()->goal.id;
        switch (habit_selected_) {
        case 0:
            if (callbacks_.on_history)
                callbacks_.on_history(id);
            break;
        case 1:
            if (callbacks_.on_edit_habit)
                callbacks_.on_edit_habit(id);
            break;
        case 2:
        case 3:
            if (callbacks_.on_move)
                callbacks_.on_move(id, habit_selected_ == 2 ? -1 : +1);
            reload(); // the selection follows the habit, so it can be moved again right away
            break;
        case 4:
            confirm_selected_ = 0;
            show_confirm_ = true;
            break;
        default:
            break;
        }
    }

    void on_confirm() {
        show_confirm_ = false;
        if (confirm_selected_ != 0)
            return;
        if (const auto* h = habit()) {
            if (callbacks_.on_delete_habit)
                callbacks_.on_delete_habit(h->goal.id);
        } else if (callbacks_.on_delete_routine) {
            callbacks_.on_delete_routine(routine().routine.id);
        }
        reload();
    }

    void reload() {
        // Keep the selection on the same heading or habit, wherever it moved.
        const auto before = lines_.empty() ? std::string() : line_key(routines_, line());
        routines_ = load_();
        lines_ = routine_lines(routines_, [](std::size_t) { return true; });
        rows_->DetachAllChildren();
        for (std::size_t i = 0; i < lines_.size(); ++i) {
            MenuEntryOption option;
            option.transform = [this, i](const EntryState& state) {
                const auto& l = lines_[i];
                const auto& r = routines_[l.routine];
                auto row = l.habit ? habit_row(r.habits[*l.habit]) : routine_row(r);
                if (state.focused)
                    row |= inverted;
                if (static_cast<int>(i) == selected_row_)
                    row |= focus; // keep the selected row visible when scrolling
                return row;
            };
            rows_->Add(MenuEntry(option));
        }
        if (const auto found = find_line(routines_, lines_, before))
            selected_row_ = static_cast<int>(*found);
        selected_row_ =
            std::clamp(selected_row_, 0, std::max(0, static_cast<int>(lines_.size()) - 1));
        if (lines_.empty())
            focus_area_ = 0;
    }

    std::function<std::vector<RoutineSummary>()> load_;
    RoutinesCallbacks callbacks_;
    std::vector<RoutineSummary> routines_;
    std::vector<RoutineLine> lines_;
    int selected_row_ = 0;
    int focus_area_ = 0; // 0 = toolbar, 1 = rows
    Component toolbar_;
    Component rows_;

    bool show_routine_actions_ = false;
    std::vector<std::string> routine_entries_{"Add habit", "Edit", "Delete", "Cancel"};
    int routine_selected_ = 0;
    bool show_habit_actions_ = false;
    std::vector<std::string> habit_entries_{"History", "Edit",   "Move up",
                                            "Move down", "Delete", "Cancel"};
    int habit_selected_ = 0;
    bool show_confirm_ = false;
    std::vector<std::string> confirm_entries_{"Delete", "Cancel"};
    int confirm_selected_ = 0;
};

} // namespace

Component make_routines_view(std::function<std::vector<RoutineSummary>()> load,
                             RoutinesCallbacks callbacks) {
    return Make<RoutinesView>(std::move(load), std::move(callbacks));
}

} // namespace ortha
