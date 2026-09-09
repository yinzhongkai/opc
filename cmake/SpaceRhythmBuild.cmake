option(SPACE_RHYTHM_WARNINGS_AS_ERRORS "Treat project warnings as errors" ON)

add_library(space_rhythm_build_options INTERFACE)
target_compile_definitions(space_rhythm_build_options INTERFACE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
target_compile_options(space_rhythm_build_options INTERFACE
    /W4
    /permissive-
    /Zc:__cplusplus
    /utf-8
)

function(space_rhythm_apply_build_policy target)
    set(options ALLOW_GENERATED_WARNINGS)
    cmake_parse_arguments(ARG "${options}" "" "" ${ARGN})

    target_link_libraries(${target} PRIVATE space_rhythm_build_options)
    if(SPACE_RHYTHM_WARNINGS_AS_ERRORS AND NOT ARG_ALLOW_GENERATED_WARNINGS)
        target_compile_options(${target} PRIVATE /WX)
    endif()
endfunction()
