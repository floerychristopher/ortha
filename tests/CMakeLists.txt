include(${catch2_SOURCE_DIR}/extras/Catch.cmake)

add_executable(ortha_tests
    core/date_test.cpp
    core/uuid_test.cpp
    core/sqlite_test.cpp
    core/schema_test.cpp
    core/recurrence_test.cpp
    core/goal_test.cpp
    core/routine_test.cpp
    core/routine_repository_test.cpp
    core/bible_verses_test.cpp
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
    tui/routine_form_test.cpp
    tui/routines_view_test.cpp
    tui/routine_lines_test.cpp
    test_names_test.cpp
    tui/goal_history_view_test.cpp
    tui/goal_widgets_test.cpp
    tui/goal_month_view_test.cpp
    tui/popup_test.cpp
)
target_link_libraries(ortha_tests PRIVATE ortha_tui Catch2::Catch2WithMain)
ortha_set_warnings(ortha_tests)

catch_discover_tests(ortha_tests)

# The program itself: --version and unknown options must answer before any database or UI.
# ORTHA_DB points below a file, so an accidental database access fails loudly.
set(ortha_no_db "ORTHA_DB=${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt/ortha.db")
string(REPLACE "." "\\." version_regex "${PROJECT_VERSION}")
add_test(NAME ortha_version
         COMMAND ${CMAKE_COMMAND} -DORTHA=$<TARGET_FILE:ortha> -DARG=--version -DRC=0
                 "-DSTDOUT=^ortha ${version_regex}\r?\n$" "-DSTDERR=^$"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_ortha.cmake)
add_test(NAME ortha_unknown_option
         COMMAND ${CMAKE_COMMAND} -DORTHA=$<TARGET_FILE:ortha> -DARG=--nope -DRC=2
                 "-DSTDOUT=^$" "-DSTDERR=ortha: unknown option '--nope'"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_ortha.cmake)
set_tests_properties(ortha_version ortha_unknown_option PROPERTIES ENVIRONMENT "${ortha_no_db}")
