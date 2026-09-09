add_library(space_rhythm_media_adapter STATIC
    media_adapter_skeleton.cpp
)
add_library(SpaceRhythm::MediaAdapter ALIAS space_rhythm_media_adapter)

target_include_directories(space_rhythm_media_adapter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(space_rhythm_media_adapter PUBLIC SpaceRhythm::Core)
space_rhythm_apply_build_policy(space_rhythm_media_adapter)

install(TARGETS space_rhythm_media_adapter ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
