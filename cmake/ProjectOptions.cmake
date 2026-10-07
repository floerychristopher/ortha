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

# Compiler warnings for ortha's own targets (not for third-party code).
function(ortha_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()
