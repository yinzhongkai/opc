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

add_library(space_rhythm_rendering_geometry STATIC
    geometry_core.cpp
)
add_library(SpaceRhythm::RenderingGeometry ALIAS space_rhythm_rendering_geometry)
target_include_directories(space_rhythm_rendering_geometry PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(space_rhythm_rendering_geometry
    PUBLIC SpaceRhythm::RenderingContract
)
space_rhythm_apply_build_policy(space_rhythm_rendering_geometry)

add_library(space_rhythm_scene_graph STATIC
    include/space_rhythm/rendering/scene_graph_render_item.hpp
    scene_graph_render_item.cpp
)
add_library(SpaceRhythm::SceneGraph ALIAS space_rhythm_scene_graph)
target_include_directories(space_rhythm_scene_graph PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(space_rhythm_scene_graph
    PUBLIC
        SpaceRhythm::RenderingGeometry
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
)
space_rhythm_apply_build_policy(space_rhythm_scene_graph)

add_library(space_rhythm_offscreen_rendering STATIC
    include/space_rhythm/rendering/offscreen_renderer.hpp
    offscreen_renderer.cpp
)
add_library(SpaceRhythm::OffscreenRendering ALIAS space_rhythm_offscreen_rendering)
target_include_directories(space_rhythm_offscreen_rendering PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(space_rhythm_offscreen_rendering
    PUBLIC
        SpaceRhythm::SceneGraph
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
    PRIVATE
        d3d11
        dxgi
        Psapi
)
space_rhythm_apply_build_policy(space_rhythm_offscreen_rendering)

install(TARGETS
    space_rhythm_rendering_contract
    space_rhythm_rendering_geometry
    space_rhythm_scene_graph
    space_rhythm_offscreen_rendering
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
