add_library(space_rhythm_rendering_contract STATIC
    render_contract.cpp
)
add_library(SpaceRhythm::RenderingContract ALIAS space_rhythm_rendering_contract)

target_include_directories(space_rhythm_rendering_contract PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(space_rhythm_rendering_contract PUBLIC SpaceRhythm::Core)
space_rhythm_apply_build_policy(space_rhythm_rendering_contract)

install(TARGETS space_rhythm_rendering_contract ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
