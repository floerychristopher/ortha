#include "tui/routine_form.hpp"

#include "core/text.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <memory>

namespace ortha {

using namespace ftxui;

RoutineFormState routine_form_state_from(const Routine& routine) {
    if (routine.id.empty())
        return {routine.name, "", "", routine.identity};
    return {routine.name, format_time(routine.start),
            routine.end ? format_time(*routine.end) : "", routine.identity};
}

std::variant<Routine, std::string> apply_routine_form(const RoutineFormState& state,
                                                      Routine base) {
    auto name = trim(state.name);
    if (name.empty())
        return std::string("Name must not be empty");
    const auto start = parse_time(state.start);
    if (!start)
        return std::string("Start must be HH:MM");
    std::optional<int> end;
    if (!trim(state.end).empty()) {
        end = parse_time(state.end);
        if (!end)
            return std::string("End must be HH:MM");
        // within the routine day, which changes at 4:00: 23:30–0:30 is fine
        if (routine_end_minutes(*end) <= routine_minutes(*start))
            return std::string("End must be after start");
    }
    base.name = std::move(name);
    base.start = *start;
    base.end = end;
    base.identity = trim(state.identity);
    return base;
}

Component make_routine_form(std::string heading, Routine initial,
                            std::function<void(Routine)> on_save,
                            std::function<void()> on_cancel) {
    struct Model {
        Routine base;
        RoutineFormState form;
        std::string error;
        int cursors[4] = {};
    };
    auto model = std::make_shared<Model>();
    model->form = routine_form_state_from(initial);
    model->base = std::move(initial);

    auto save = [model, on_save] {
        auto result = apply_routine_form(model->form, model->base);
        if (auto* error = std::get_if<std::string>(&result)) {
            model->error = *error;
            return;
        }
        model->error.clear();
        on_save(std::get<Routine>(std::move(result)));
    };

    auto field = [&](std::string* content, int* cursor, std::string placeholder) {
        *cursor = static_cast<int>(content->size());
        auto option = InputOption::Default();
        option.content = content;
        option.placeholder = std::move(placeholder);
        option.multiline = false;
        option.cursor_position = cursor;
        option.on_enter = save;
        return Input(option);
    };
    auto name = field(&model->form.name, &model->cursors[0], "e.g. morning routine");
    auto start = field(&model->form.start, &model->cursors[1], "hh:mm");
    auto end = field(&model->form.end, &model->cursors[2], "hh:mm (optional)");
    auto identity =
        field(&model->form.identity, &model->cursors[3], "e.g. i start the day with prayer");

    auto save_button = Button("Save", save, ButtonOption::Ascii());
    auto cancel_button = Button("Cancel", on_cancel, ButtonOption::Ascii());
    auto layout = Container::Vertical({
        name,
        start,
        end,
        identity,
        Container::Horizontal({save_button, cancel_button}),
    });

    auto view = Renderer(layout, [=] {
        auto labeled = [](const char* label, Element field) {
            return hbox({text(label) | size(WIDTH, EQUAL, 10), std::move(field) | flex});
        };
        Elements rows = {
            labeled("Name", name->Render()),
            labeled("Start", start->Render()),
            labeled("End", end->Render()),
            labeled("Identity", identity->Render()),
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
        // Like the other forms: Tab moves between fields.
        if (event == Event::Tab)
            return layout->OnEvent(Event::ArrowDown);
        if (event == Event::TabReverse)
            return layout->OnEvent(Event::ArrowUp);
        return false;
    });
}

} // namespace ortha
