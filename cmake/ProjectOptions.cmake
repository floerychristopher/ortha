# Sanitizers apply to everything (including third-party code), so they are set globally
# before the dependencies are added.
if(ORTHA_SANITIZE)
    if(MSVC)
        message(FATAL_ERROR "ORTHA_SANITIZE is only supported with GCC and Clang")
    endif()
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer
                        -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address,undefined)
endif()

# Link the C++ runtime statically, so ortha runs on other machines without extra DLLs/.so files:
# MSVC needs no VC++ Redistributable, MinGW no libstdc++/libgcc/winpthread DLLs, and a Linux
# build runs on any distro whose glibc is at least as new as the build machine's.
if(NOT ORTHA_SANITIZE)
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    elseif(MINGW)
        add_link_options(-static)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_link_options(-static-libstdc++ -static-libgcc)
    endif()
endif()

# Compiler warnings for ortha's own targets (not for third-party code).
function(ortha_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()
