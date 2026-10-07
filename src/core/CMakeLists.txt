add_library(ortha_core STATIC
    date.cpp
    uuid.cpp
    sqlite.cpp
    schema.cpp
    recurrence.cpp
    goal.cpp
    task.cpp
    task_repository.cpp
    task_service.cpp
    paths.cpp
)
target_include_directories(ortha_core PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(ortha_core PRIVATE sqlite3)
ortha_set_warnings(ortha_core)
