include(${catch2_SOURCE_DIR}/extras/Catch.cmake)

add_executable(ortha_tests
    core/date_test.cpp
    core/uuid_test.cpp
    core/sqlite_test.cpp
)
target_link_libraries(ortha_tests PRIVATE ortha_core Catch2::Catch2WithMain)
ortha_set_warnings(ortha_tests)

catch_discover_tests(ortha_tests)
