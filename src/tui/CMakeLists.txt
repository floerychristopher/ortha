add_library(ortha_tui STATIC
    task_form_state.cpp
    task_form.cpp
    task_view.cpp
    task_list_view.cpp
    home_view.cpp
    error_guard.cpp
    goal_form.cpp
    goals_view.cpp
    app.cpp
)
target_link_libraries(ortha_tui PUBLIC ortha_core ftxui::screen ftxui::dom ftxui::component)
ortha_set_warnings(ortha_tui)

add_executable(ortha main.cpp)
target_link_libraries(ortha PRIVATE ortha_tui)
ortha_set_warnings(ortha)
