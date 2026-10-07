add_library(ortha_tui STATIC
    task_form_state.cpp
    task_form.cpp
)
target_link_libraries(ortha_tui PUBLIC ortha_core ftxui::screen ftxui::dom ftxui::component)
ortha_set_warnings(ortha_tui)
