include(${catch2_SOURCE_DIR}/extras/Catch.cmake)

add_executable(ortha_tests
    core/date_test.cpp
    core/uuid_test.cpp
    core/sqlite_test.cpp
    core/schema_test.cpp
    core/recurrence_test.cpp
    core/goal_test.cpp
    core/goal_repository_test.cpp
    core/task_repository_test.cpp
    core/task_service_test.cpp
    core/paths_test.cpp
    tui/task_form_state_test.cpp
    tui/task_form_test.cpp
    tui/task_list_view_test.cpp
    tui/home_view_test.cpp
    tui/error_guard_test.cpp
    tui/goal_form_test.cpp
    tui/goals_view_test.cpp
    tui/goal_history_view_test.cpp
)
target_link_libraries(ortha_tests PRIVATE ortha_tui Catch2::Catch2WithMain)
ortha_set_warnings(ortha_tests)

catch_discover_tests(ortha_tests)
