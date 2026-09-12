add_library(space_rhythm_ui_bridge STATIC
    include/space_rhythm/ui/scene_graph_attachment.hpp
    include/space_rhythm/ui/view_models.hpp
    include/space_rhythm/ui/workspace_service.hpp
    integrated_workspace_service.cpp
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
        SpaceRhythm::MediaAdapter
        SpaceRhythm::AudioAnalysis
        SpaceRhythm::AudioRender
        SpaceRhythm::PlaybackExport
        SpaceRhythm::OffscreenRendering
        SpaceRhythm::UiWorkerProtocol
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
    PRIVATE
        SpaceRhythm::AudioPreviewQt
        Qt6::Multimedia
)

qt_add_resources(space_rhythm_ui_bridge "ui_development_timbres"
    PREFIX "/space-rhythm/development-timbres"
    BASE "${PROJECT_SOURCE_DIR}/tests/golden/audio/generated"
    FILES
        "${PROJECT_SOURCE_DIR}/tests/golden/audio/generated/timbre-click-48000-mono.f32le"
        "${PROJECT_SOURCE_DIR}/tests/golden/audio/generated/timbre-low-triangle-48000-mono.f32le"
        "${PROJECT_SOURCE_DIR}/tests/golden/audio/generated/timbre-noise-hit-48000-mono.f32le"
)
space_rhythm_apply_build_policy(space_rhythm_ui_bridge)

install(TARGETS space_rhythm_ui_bridge ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
