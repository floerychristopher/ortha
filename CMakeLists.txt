cmake_minimum_required(VERSION 3.25)
project(ortha VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ORTHA_BUILD_TESTS "Build the ortha unit tests" ON)
option(ORTHA_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer (GCC/Clang)" OFF)

include(cmake/ProjectOptions.cmake)
include(cmake/Dependencies.cmake)

add_subdirectory(src/core)
add_subdirectory(src/tui)

if(ORTHA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
