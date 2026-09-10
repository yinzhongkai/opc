add_library(space_rhythm_media_adapter STATIC
    ffmpeg_media.cpp
    media_adapter_skeleton.cpp
)
add_library(SpaceRhythm::MediaAdapter ALIAS space_rhythm_media_adapter)

target_include_directories(space_rhythm_media_adapter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(space_rhythm_media_adapter SYSTEM PRIVATE
    ${FFMPEG_INCLUDE_DIRS}
)
target_link_libraries(space_rhythm_media_adapter
    PUBLIC SpaceRhythm::Core
    PRIVATE ${FFMPEG_LIBRARIES}
)
space_rhythm_apply_build_policy(space_rhythm_media_adapter)

install(TARGETS space_rhythm_media_adapter ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

file(GLOB _space_rhythm_ffmpeg_release_dlls CONFIGURE_DEPENDS
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/avcodec-*.dll"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/avdevice-*.dll"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/avfilter-*.dll"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/avformat-*.dll"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/avutil-*.dll"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/swresample-*.dll"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/swscale-*.dll"
)
file(GLOB _space_rhythm_ffmpeg_debug_dlls CONFIGURE_DEPENDS
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/avcodec-*.dll"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/avdevice-*.dll"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/avfilter-*.dll"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/avformat-*.dll"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/avutil-*.dll"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/swresample-*.dll"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/swscale-*.dll"
)
install(FILES ${_space_rhythm_ffmpeg_release_dlls}
    DESTINATION bin
    CONFIGURATIONS Release RelWithDebInfo MinSizeRel
)
install(FILES ${_space_rhythm_ffmpeg_debug_dlls}
    DESTINATION bin
    CONFIGURATIONS Debug
)
