#include "tui/goal_form.hpp"

#include "core/text.hpp"
#include "tui/goal_widgets.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <memory>

namespace ortha {

using namespace ftxui;

GoalFormState goal_form_state_from(const Goal& goal) {
    return {goal.title, goal.weekdays, goal.symbol};
}

std::variant<Goal, std::string> apply_goal_form(const GoalFormState& state, Goal base) {
    auto title = trim(state.title);
    if (title.empty())
        return std::string("Title must not be empty");
    if (std::none_of(state.weekdays.begin(), state.weekdays.end(), [](bool d) { return d; }))
        return std::string("Choose at least one day");
    auto symbol = trim(state.symbol);
    if (ftxui::string_width(symbol) > kMaxSymbolWidth)
        return std::string("Symbol is too wide (max. 2 emoji or 4 characters)");
    base.title = std::move(title);
    base.symbol = std::move(symbol);
    base.weekdays = state.weekdays;
    return base;
}

Component make_goal_form(std::string heading, Goal initial, std::function<void(Goal)> on_save,
                         std::function<void()> on_cancel) {
    struct Model {
        Goal base;
        GoalFormState form;
        std::string error;
        int title_cursor = 0;
        int symbol_cursor = 0;
    };
    auto model = std::make_shared<Model>();
    model->form = goal_form_state_from(initial);
    model->base = std::move(initial);
    model->title_cursor = static_cast<int>(model->form.title.size());
    model->symbol_cursor = static_cast<int>(model->form.symbol.size());

    auto save = [model, on_save] {
        auto result = apply_goal_form(model->form, model->base);
        if (auto* error = std::get_if<std::string>(&result)) {
            model->error = *error;
            return;
        }
        model->error.clear();
        on_save(std::get<Goal>(std::move(result)));
    };

    auto title_option = InputOption::Default();
    title_option.content = &model->form.title;
    title_option.placeholder = "e.g. Go to the gym";
    title_option.multiline = false;
    title_option.cursor_position = &model->title_cursor;
    title_option.on_enter = save;
    auto title = Input(title_option);

    auto symbol_option = InputOption::Default();
    symbol_option.content = &model->form.symbol;
    symbol_option.placeholder = "e.g. 💪";
    symbol_option.multiline = false;
    symbol_option.cursor_position = &model->symbol_cursor;
    symbol_option.on_enter = save;
    auto symbol = Input(symbol_option);

    static constexpr const char* kDayNames[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
    Components day_boxes;
    for (std::size_t i = 0; i < 7; ++i)
        day_boxes.push_back(Checkbox(kDayNames[i], &model->form.weekdays[i]));
    auto days = Container::Horizontal(day_boxes);

    auto save_button = Button("Save", save, ButtonOption::Ascii());
    auto cancel_button = Button("Cancel", on_cancel, ButtonOption::Ascii());
    auto layout = Container::Vertical({
        title,
        symbol,
        days,
        Container::Horizontal({save_button, cancel_button}),
    });

    auto view = Renderer(layout, [=] {
        Elements boxes;
        for (const auto& box : day_boxes) {
            boxes.push_back(box->Render());
            boxes.push_back(text("  "));
        }
        Elements rows = {
            hbox({text("Title") | size(WIDTH, EQUAL, 10), title->Render() | flex}),
            hbox({text("Symbol") | size(WIDTH, EQUAL, 10), symbol->Render() | flex}),
            hbox({text("Days") | size(WIDTH, EQUAL, 10), hbox(std::move(boxes))}),
            separator(),
            hbox({save_button->Render(), text("  "), cancel_button->Render()}),
        };
        if (!model->error.empty())
            rows.push_back(text(model->error) | color(Color::Red));
        return window(text(" " + heading + " ") | bold, vbox(std::move(rows))) |
               size(WIDTH, LESS_THAN, 80);
    });

    return CatchEvent(view, [on_cancel, layout](const Event& event) {
        if (event == Event::Escape) {
            on_cancel();
            return true;
        }
        // Like the task form: Tab moves between fields instead of inside the day boxes.
        if (event == Event::Tab)
            return layout->OnEvent(Event::ArrowDown);
        if (event == Event::TabReverse)
            return layout->OnEvent(Event::ArrowUp);
        return false;
    });
}

} // namespace ortha
