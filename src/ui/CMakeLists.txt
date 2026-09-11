add_library(space_rhythm_ui_bridge STATIC
    include/space_rhythm/ui/scene_graph_attachment.hpp
    include/space_rhythm/ui/view_models.hpp
    include/space_rhythm/ui/workspace_service.hpp
    mock_workspace_service.cpp
    scene_graph_attachment.cpp
    view_models.cpp
)
add_library(SpaceRhythm::UiBridge ALIAS space_rhythm_ui_bridge)

target_include_directories(space_rhythm_ui_bridge PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(space_rhythm_ui_bridge
    PUBLIC
        SpaceRhythm::Core
        SpaceRhythm::System
        SpaceRhythm::RenderingGeometry
        SpaceRhythm::SceneGraph
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)
space_rhythm_apply_build_policy(space_rhythm_ui_bridge)

install(TARGETS space_rhythm_ui_bridge ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
