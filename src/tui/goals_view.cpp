#include "tui/goals_view.hpp"

#include "tui/goal_widgets.hpp"
#include "tui/popup.hpp"

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <format>

namespace ortha {

using namespace ftxui;

namespace {

Element week_strip(const std::array<std::optional<GoalStatus>, 7>& week) {
    Elements days;
    for (const auto& day : week) {
        if (!day)
            days.push_back(text("·") | dim);
        else if (*day == GoalStatus::Done)
            days.push_back(text("✓") | color(Color::Green));
        else if (*day == GoalStatus::Missed)
            days.push_back(text("✗") | color(Color::Red));
        else
            days.push_back(text("○"));
    }
    return hbox(std::move(days));
}

Element goal_row(const GoalSummary& goal, bool focused, bool selected) {
    auto row = hbox({
        text(goal_symbol(goal.goal) + " " + goal.goal.title) | flex_shrink,
        filler(),
        text("  " + weekdays_label(goal.goal.weekdays)) | size(WIDTH, EQUAL, 22),
        week_strip(goal.last_week),
        text(goal.streak > 0 ? std::format("  🔥 {}", goal.streak) : "") | size(WIDTH, EQUAL, 8),
    });
    if (focused)
        row |= inverted;
    if (selected)
        row |= focus; // keep the selected row visible when scrolling
    return row;
}

class GoalsView : public ComponentBase {
  public:
    GoalsView(std::function<std::vector<GoalSummary>()> load, GoalsCallbacks callbacks)
        : load_(std::move(load)), callbacks_(std::move(callbacks)) {
        auto back = Button("Back", [this] { go_back(); }, ButtonOption::Ascii());
        auto add = Button(
            "New goal", [this] { callbacks_.on_new ? callbacks_.on_new() : void(); },
            ButtonOption::Ascii());
        auto month = Button(
            "Month", [this] { callbacks_.on_month ? callbacks_.on_month() : void(); },
            ButtonOption::Ascii());
        toolbar_ = Container::Horizontal({back, add, month});
        rows_ = Container::Vertical({}, &selected_row_);

        auto body = Renderer(Container::Vertical({toolbar_, rows_}, &focus_area_), [this] {
            Element list = goals_.empty()
                               ? text("No goals yet.") | dim
                               : rows_->Render() | vscroll_indicator | yframe | flex;
            return window(text(" Goals ") | bold, vbox({toolbar_->Render(), separator(), list}));
        });
        auto actions = popup_menu(&action_entries_, &action_selected_, [this] { on_action(); });
        auto confirm =
            popup_menu(&confirm_entries_, &confirm_selected_, [this] { on_confirm(); });
        auto actions_popup = Renderer(actions, [this, actions] {
            return window(text(" " + selected_goal().goal.title + " "), actions->Render());
        });
        auto confirm_popup = Renderer(confirm, [this, confirm] {
            return window(text(" Delete \"" + selected_goal().goal.title + "\"? "),
                          confirm->Render());
        });
        Add(Modal(Modal(body, actions_popup, &show_actions_), confirm_popup, &show_confirm_));

        reload();
        focus_area_ = goals_.empty() ? 0 : 1;
    }

    bool OnEvent(Event event) override {
        if (event == Event::Escape) {
            if (show_actions_ || show_confirm_)
                show_actions_ = show_confirm_ = false;
            else
                go_back();
            return true;
        }
        if (event == Event::Return && !show_actions_ && !show_confirm_ && focus_area_ == 1 &&
            !goals_.empty()) {
            action_selected_ = 0;
            show_actions_ = true;
            return true;
        }
        return ComponentBase::OnEvent(event);
    }

  private:
    const GoalSummary& selected_goal() const {
        static const GoalSummary none{};
        return goals_.empty() ? none : goals_[static_cast<std::size_t>(selected_row_)];
    }

    void go_back() {
        if (callbacks_.on_back)
            callbacks_.on_back();
    }

    void on_action() {
        show_actions_ = false;
        const auto id = selected_goal().goal.id;
        switch (action_selected_) {
        case 0:
            if (callbacks_.on_history)
                callbacks_.on_history(id);
            break;
        case 1:
            if (callbacks_.on_edit)
                callbacks_.on_edit(id);
            break;
        case 2:
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
        const auto id = selected_goal().goal.id;
        if (callbacks_.on_delete)
            callbacks_.on_delete(id);
        reload();
    }

    void reload() {
        goals_ = load_();
        rows_->DetachAllChildren();
        for (std::size_t i = 0; i < goals_.size(); ++i) {
            MenuEntryOption option;
            option.label = goals_[i].goal.title;
            option.transform = [this, i](const EntryState& state) {
                return goal_row(goals_[i], state.focused, static_cast<int>(i) == selected_row_);
            };
            rows_->Add(MenuEntry(option));
        }
        selected_row_ =
            std::clamp(selected_row_, 0, std::max(0, static_cast<int>(goals_.size()) - 1));
        if (goals_.empty())
            focus_area_ = 0;
    }

    std::function<std::vector<GoalSummary>()> load_;
    GoalsCallbacks callbacks_;
    std::vector<GoalSummary> goals_;
    int selected_row_ = 0;
    int focus_area_ = 0; // 0 = toolbar, 1 = rows
    Component toolbar_;
    Component rows_;

    bool show_actions_ = false;
    std::vector<std::string> action_entries_{"History", "Edit", "Delete", "Cancel"};
    int action_selected_ = 0;
    bool show_confirm_ = false;
    std::vector<std::string> confirm_entries_{"Delete", "Cancel"};
    int confirm_selected_ = 0;
};

} // namespace

Component make_goals_view(std::function<std::vector<GoalSummary>()> load,
                          GoalsCallbacks callbacks) {
    return Make<GoalsView>(std::move(load), std::move(callbacks));
}

} // namespace ortha
