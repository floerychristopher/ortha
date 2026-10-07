#include "tui/task_form.hpp"

#include "tui/task_form_state.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <memory>
#include <vector>

namespace ortha {

using namespace ftxui;

namespace {

struct FormModel {
    Task base;
    TaskFormState form;
    std::string error;
    std::vector<std::string> priorities{"none", "low", "medium", "high"};
    std::vector<std::string> repeats{"none", "daily", "weekly", "monthly", "yearly"};
    std::vector<std::string> anchors{"due date", "completion"};
    int interval_cursor = 0;
    // Cursor positions; start at the end of the prefilled text so typing appends.
    int title_cursor = 0;
    int due_cursor = 0;
    int tags_cursor = 0;
    std::string new_subtask;
    int new_subtask_cursor = 0;
};

Component single_line_input(std::string* content, int* cursor, std::string placeholder,
                            std::function<void()> on_enter) {
    *cursor = static_cast<int>(content->size());
    auto option = InputOption::Default();
    option.content = content;
    option.cursor_position = cursor;
    option.placeholder = std::move(placeholder);
    option.multiline = false;
    option.on_enter = std::move(on_enter);
    return Input(option);
}

Element labeled(const std::string& label, Element field) {
    return hbox({text(label) | size(WIDTH, EQUAL, 10), field | flex});
}

} // namespace

Component make_task_form(std::string heading, Task initial, std::function<void(Task)> on_save,
                         std::function<void()> on_cancel) {
    auto model = std::make_shared<FormModel>();
    model->form = form_state_from(initial);
    model->base = std::move(initial);

    auto save = [model, on_save] {
        auto result = apply_form(model->form, model->base);
        if (auto* error = std::get_if<std::string>(&result)) {
            model->error = *error;
            return;
        }
        model->error.clear();
        on_save(std::get<Task>(std::move(result)));
    };

    auto title = single_line_input(&model->form.title, &model->title_cursor, "what needs to be done?", save);
    auto due = single_line_input(&model->form.due, &model->due_cursor, "dd.mm.yyyy", save);
    auto priority = Toggle(&model->priorities, &model->form.priority);
    auto tags = single_line_input(&model->form.tags, &model->tags_cursor, "comma separated", save);
    auto notes_option = InputOption::Default();
    notes_option.content = &model->form.notes;
    notes_option.placeholder = "notes (enter for new line)";
    auto notes = Input(notes_option);

    // Subtasks: one row per subtask (checkbox + remove button), rebuilt whenever the list changes.
    // The "add subtask" input keeps the rebuild function alive; the remove buttons only hold a
    // weak_ptr to it, so there is no ownership cycle.
    auto subtask_rows = Container::Vertical({});
    auto rebuild_subtasks = std::make_shared<std::function<void()>>();
    *rebuild_subtasks = [model, subtask_rows, weak = std::weak_ptr(rebuild_subtasks)] {
        subtask_rows->DetachAllChildren();
        for (std::size_t i = 0; i < model->form.subtasks.size(); ++i) {
            auto& sub = model->form.subtasks[i];
            auto remove = Button(
                "Remove",
                [model, i, weak] {
                    model->form.subtasks.erase(model->form.subtasks.begin() +
                                               static_cast<std::ptrdiff_t>(i));
                    if (auto rebuild = weak.lock())
                        (*rebuild)();
                },
                ButtonOption::Ascii());
            auto check = Checkbox(sub.title, &sub.done);
            subtask_rows->Add(Renderer(Container::Horizontal({check, remove}), [check, remove] {
                return hbox({check->Render(), text("  "), remove->Render()});
            }));
        }
    };
    (*rebuild_subtasks)();
    auto new_subtask = single_line_input(
        &model->new_subtask, &model->new_subtask_cursor, "add subtask (enter)",
        [model, rebuild_subtasks] {
            if (model->new_subtask.find_first_not_of(" \t") == std::string::npos)
                return;
            model->form.subtasks.push_back({"", model->new_subtask, false});
            model->new_subtask.clear();
            model->new_subtask_cursor = 0;
            (*rebuild_subtasks)();
        });

    // Repeat: frequency toggle, then interval/weekdays/anchor only while repeating.
    auto repeat = Toggle(&model->repeats, &model->form.repeat);
    auto interval = single_line_input(&model->form.interval, &model->interval_cursor, "1", save);
    Components day_boxes;
    for (std::size_t i = 0; i < 7; ++i)
        day_boxes.push_back(Checkbox(kWeekdayNames[i], &model->form.weekdays[i]));
    auto weekdays = Container::Horizontal(day_boxes);
    auto anchor = Toggle(&model->anchors, &model->form.anchor);
    auto repeating = [model] { return model->form.repeat != 0; };
    auto weekly = [model] {
        return model->form.repeat == static_cast<int>(Frequency::Weekly) + 1;
    };
    auto repeat_details = Container::Vertical({
        interval,
        weekdays | Maybe(weekly),
        anchor,
    });

    auto save_button = Button("Save", save, ButtonOption::Ascii());
    auto cancel_button = Button("Cancel", on_cancel, ButtonOption::Ascii());

    auto layout = Container::Vertical({
        title,
        due,
        priority,
        tags,
        notes,
        repeat,
        repeat_details | Maybe(repeating),
        subtask_rows,
        new_subtask,
        Container::Horizontal({save_button, cancel_button}),
    });

    auto view = Renderer(layout, [=] {
        Elements rows = {
            labeled("Title", title->Render()),
            labeled("Due", due->Render()),
            labeled("Priority", priority->Render()),
            labeled("Tags", tags->Render()),
            labeled("Notes", notes->Render() | size(HEIGHT, LESS_THAN, 6)),
            labeled("Repeat", repeat->Render()),
        };
        if (repeating()) {
            static constexpr const char* kUnits[] = {"days", "weeks", "months", "years"};
            rows.push_back(labeled("  Every",
                                   hbox({interval->Render() | size(WIDTH, EQUAL, 4),
                                         text(std::string(" ") + kUnits[model->form.repeat - 1])})));
            if (weekly()) {
                Elements boxes;
                for (const auto& box : day_boxes) {
                    boxes.push_back(box->Render());
                    boxes.push_back(text("  "));
                }
                rows.push_back(labeled("  On", hbox(std::move(boxes))));
            }
            rows.push_back(labeled("  From", anchor->Render()));
        }
        rows.push_back(labeled(
            "Subtasks", vbox({model->form.subtasks.empty() ? emptyElement() : subtask_rows->Render(),
                              new_subtask->Render()})));
        rows.push_back(separator());
        rows.push_back(hbox({save_button->Render(), text("  "), cancel_button->Render()}));
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
        // Tab behaves like the arrow keys, so it cannot get stuck inside the priority toggle
        // (which would otherwise use Tab to cycle its own entries).
        if (event == Event::Tab)
            return layout->OnEvent(Event::ArrowDown);
        if (event == Event::TabReverse)
            return layout->OnEvent(Event::ArrowUp);
        return false;
    });
}

} // namespace ortha
