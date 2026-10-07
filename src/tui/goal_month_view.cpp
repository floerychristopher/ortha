#include "tui/goal_month_view.hpp"

#include "tui/goal_widgets.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <format>

namespace ortha {

using namespace ftxui;
namespace ch = std::chrono;

namespace {

constexpr const char* kDayNames[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
constexpr const char* kMonthNames[] = {"January", "February", "March",     "April",
                                       "May",     "June",     "July",      "August",
                                       "September", "October", "November", "December"};

std::string month_name(ch::month month) { return kMonthNames[static_cast<unsigned>(month) - 1]; }

constexpr int kDayLabelWidth = 5;

// "We 07"
std::string day_label(Date day) {
    return std::format("{} {:02}", kDayNames[weekday_index(day)],
                       static_cast<unsigned>(day.day()));
}

// Centers `content` in a column as wide as the widest symbol.
Element column(Element content) {
    return hbox({filler(), std::move(content), filler()}) | size(WIDTH, EQUAL, kMaxSymbolWidth);
}

Element cell_mark(CellState state) {
    switch (state) {
    case CellState::NotScheduled:
        return text("·") | dim;
    case CellState::Open:
        return text("○");
    case CellState::Done:
        return text("✓") | color(Color::Green);
    case CellState::Missed:
        return text("✗") | color(Color::Red);
    default:
        return text(""); // Future
    }
}

class GoalMonthView : public ComponentBase {
  public:
    GoalMonthView(std::function<std::vector<MonthColumn>(ch::year_month)> load, Date today,
                  std::function<void(const std::string&, Date)> on_cycle,
                  std::function<void()> on_back)
        : load_(std::move(load)), today_(today), on_cycle_(std::move(on_cycle)),
          on_back_(std::move(on_back)), month_(today.year() / today.month()),
          row_(static_cast<int>(static_cast<unsigned>(today.day())) - 1) {
        auto back = Button("Back", [this] { go_back(); }, ButtonOption::Ascii());
        auto prev = Button(&prev_label_, [this] { change_month(-1); }, ButtonOption::Ascii());
        auto next = Button(&next_label_, [this] { change_month(1); }, ButtonOption::Ascii());
        toolbar_ = Container::Horizontal({back, prev, next});
        grid_ = Make<Grid>(*this);
        auto layout = Container::Vertical({toolbar_, grid_}, &focus_area_);
        Add(Renderer(layout, [this] { return render(); }));

        reload();
        focus_area_ = columns_.empty() ? 0 : 1;
    }

    bool OnEvent(Event event) override {
        if (event == Event::Escape) {
            go_back();
            return true;
        }
        if (event == Event::PageUp || event == Event::PageDown) {
            change_month(event == Event::PageUp ? -1 : 1);
            return true;
        }
        return ComponentBase::OnEvent(event);
    }

  private:
    // The cells. Arrow keys that would leave the grid are not handled, so the
    // surrounding container can move the focus into the toolbar.
    class Grid : public ComponentBase {
      public:
        explicit Grid(GoalMonthView& view) : view_(view) {}
        bool Focusable() const override { return !view_.columns_.empty(); }
        bool OnEvent(Event event) override { return view_.on_grid_event(event); }

      private:
        GoalMonthView& view_;
    };

    bool on_grid_event(const Event& event) {
        if (columns_.empty())
            return false;
        const int last_row = static_cast<int>(days_.size()) - 1;
        const int last_col = static_cast<int>(columns_.size()) - 1;
        if (event == Event::ArrowUp && row_ > 0)
            --row_;
        else if (event == Event::ArrowDown && row_ < last_row)
            ++row_;
        else if (event == Event::ArrowLeft && col_ > 0)
            --col_;
        else if (event == Event::ArrowRight && col_ < last_col)
            ++col_;
        else if (event == Event::Return)
            cycle_focused();
        else
            return false;
        return true;
    }

    CellState cell(std::size_t col, Date day) const {
        const auto& column = columns_[col];
        const auto it = column.entries.find(day);
        const auto status = it == column.entries.end() ? GoalStatus::Open : it->second;
        return month_cell(column.goal, day, today_, status);
    }

    void cycle_focused() {
        const auto col = static_cast<std::size_t>(col_);
        const auto day = days_[static_cast<std::size_t>(row_)];
        const auto state = cell(col, day);
        if (state == CellState::Future || state == CellState::NotScheduled)
            return;
        if (on_cycle_)
            on_cycle_(columns_[col].goal.id, day);
        reload();
    }

    void change_month(int delta) {
        month_ += ch::months{delta};
        reload();
    }

    void go_back() {
        if (on_back_)
            on_back_();
    }

    void reload() {
        columns_ = load_(month_);
        days_ = days_of_month(month_);
        row_ = std::clamp(row_, 0, static_cast<int>(days_.size()) - 1);
        col_ = std::clamp(col_, 0, std::max(0, static_cast<int>(columns_.size()) - 1));
        prev_label_ = "< " + month_name((month_ - ch::months{1}).month()).substr(0, 3);
        next_label_ = month_name((month_ + ch::months{1}).month()).substr(0, 3) + " >";
        if (columns_.empty())
            focus_area_ = 0;
    }

    // The header row stays in place while the day rows scroll vertically; both scroll
    // horizontally together. Every cell has a fixed width, so the columns line up.
    Element render_grid() const {
        Elements header = {text("") | size(WIDTH, EQUAL, kDayLabelWidth), text("  ")};
        for (const auto& column_data : columns_) {
            header.push_back(column(text(goal_symbol(column_data.goal))));
            header.push_back(text(" "));
        }

        Elements lines;
        for (std::size_t r = 0; r < days_.size(); ++r) {
            const auto day = days_[r];
            Elements line = {text(day_label(day)) | size(WIDTH, EQUAL, kDayLabelWidth),
                             text("  ")};
            for (std::size_t c = 0; c < columns_.size(); ++c) {
                auto mark = column(cell_mark(cell(c, day)));
                if (static_cast<int>(r) == row_ && static_cast<int>(c) == col_) {
                    if (grid_->Focused())
                        mark |= inverted;
                    mark |= focus; // keep the cursor visible when scrolling
                }
                line.push_back(std::move(mark));
                line.push_back(text(" "));
            }
            if (day == today_) {
                for (auto& element : line)
                    element |= bold;
            }
            lines.push_back(hbox(std::move(line)));
        }
        return vbox({hbox(std::move(header)), vbox(std::move(lines)) | yframe | flex}) | xframe;
    }

    Element render_footer() const {
        const auto& goal = columns_[static_cast<std::size_t>(col_)].goal;
        const auto day = days_[static_cast<std::size_t>(row_)];
        auto cursor = text(goal_symbol(goal) + " " + goal.title + " · " +
                           kDayNames[weekday_index(day)] + " " + format_date(day));
        Elements legend;
        for (const auto& column_data : columns_)
            legend.push_back(text(goal_symbol(column_data.goal) + " " + column_data.goal.title));
        return vbox({cursor, flexbox(std::move(legend), FlexboxConfig().SetGap(2, 0)) | dim});
    }

    Element render() const {
        const auto title = std::format(" {} {} ", month_name(month_.month()),
                                       static_cast<int>(month_.year()));
        Element body = columns_.empty()
                           ? text("No goals yet.") | dim
                           : vbox({render_grid() | flex, separator(), render_footer()});
        return window(text(title) | bold, vbox({toolbar_->Render(), separator(), body | flex}));
    }

    std::function<std::vector<MonthColumn>(ch::year_month)> load_;
    Date today_;
    std::function<void(const std::string&, Date)> on_cycle_;
    std::function<void()> on_back_;

    ch::year_month month_;
    std::vector<Date> days_;
    std::vector<MonthColumn> columns_;
    int row_ = 0; // index into days_
    int col_ = 0; // index into columns_
    int focus_area_ = 0; // 0 = toolbar, 1 = grid
    std::string prev_label_;
    std::string next_label_;
    Component toolbar_;
    Component grid_;
};

} // namespace

Component make_goal_month(std::function<std::vector<MonthColumn>(ch::year_month)> load,
                          Date today, std::function<void(const std::string&, Date)> on_cycle,
                          std::function<void()> on_back) {
    return Make<GoalMonthView>(std::move(load), today, std::move(on_cycle), std::move(on_back));
}

} // namespace ortha
