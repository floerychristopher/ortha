add_library(ortha_core STATIC
    date.cpp
    uuid.cpp
    sqlite.cpp
    schema.cpp
    recurrence.cpp
    goal.cpp
    routine.cpp
    routine_repository.cpp
    goal_repository.cpp
    bible_verses.cpp
    task.cpp
    task_repository.cpp
    task_service.cpp
    paths.cpp
)
# core/version.hpp: the project version as a constant
configure_file(version.hpp.in ${PROJECT_BINARY_DIR}/generated/core/version.hpp @ONLY)
target_include_directories(ortha_core PUBLIC ${PROJECT_SOURCE_DIR}/src
                                             ${PROJECT_BINARY_DIR}/generated)
target_link_libraries(ortha_core PRIVATE sqlite3)
ortha_set_warnings(ortha_core)
