add_library(space_rhythm_playback_export STATIC
    playback_export.cpp
)
add_library(SpaceRhythm::PlaybackExport ALIAS space_rhythm_playback_export)

target_include_directories(space_rhythm_playback_export PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(space_rhythm_playback_export SYSTEM PRIVATE
    ${FFMPEG_INCLUDE_DIRS}
)
target_link_libraries(space_rhythm_playback_export
    PUBLIC
        SpaceRhythm::Core
        SpaceRhythm::MediaAdapter
        SpaceRhythm::AudioRender
        SpaceRhythm::RenderingContract
    PRIVATE
        ${FFMPEG_LIBRARIES}
)
space_rhythm_apply_build_policy(space_rhythm_playback_export)

install(TARGETS space_rhythm_playback_export ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
