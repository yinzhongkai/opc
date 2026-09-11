add_library(space_rhythm_audio_render STATIC
    render.cpp
)
add_library(SpaceRhythm::AudioRender ALIAS space_rhythm_audio_render)

target_include_directories(space_rhythm_audio_render PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(space_rhythm_audio_render PUBLIC SpaceRhythm::Core)
space_rhythm_apply_build_policy(space_rhythm_audio_render)

add_library(space_rhythm_audio_preview_qt STATIC
    preview_qt.cpp
)
add_library(SpaceRhythm::AudioPreviewQt ALIAS space_rhythm_audio_preview_qt)
target_include_directories(space_rhythm_audio_preview_qt PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(space_rhythm_audio_preview_qt
    PUBLIC
        SpaceRhythm::AudioRender
        Qt6::Core
        Qt6::Multimedia
)
space_rhythm_apply_build_policy(space_rhythm_audio_preview_qt)

install(TARGETS space_rhythm_audio_render space_rhythm_audio_preview_qt
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
