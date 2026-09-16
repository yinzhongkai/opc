add_library(space_rhythm_core STATIC
    core_skeleton.cpp
    timeline.cpp
)
add_library(SpaceRhythm::Core ALIAS space_rhythm_core)

target_include_directories(space_rhythm_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
space_rhythm_apply_build_policy(space_rhythm_core)

install(TARGETS space_rhythm_core ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
