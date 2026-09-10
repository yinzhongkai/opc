find_package(kissfft CONFIG REQUIRED)
if(NOT KISSFFT_VERSION VERSION_EQUAL "131.1.0")
    message(FATAL_ERROR
        "The pinned kissfft 131.2.0 vcpkg port must expose upstream CMake package 131.1.0; "
        "found ${KISSFFT_VERSION}")
endif()

add_library(space_rhythm_audio_analysis STATIC
    analysis.cpp
)
add_library(SpaceRhythm::AudioAnalysis ALIAS space_rhythm_audio_analysis)

target_include_directories(space_rhythm_audio_analysis PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(space_rhythm_audio_analysis
    PUBLIC
        SpaceRhythm::Core
        SpaceRhythm::MediaAdapter
    PRIVATE
        kissfft::kissfft-float
)
target_compile_definitions(space_rhythm_audio_analysis PRIVATE
    SPACE_RHYTHM_KISSFFT_VERSION="131.2.0"
    SPACE_RHYTHM_KISSFFT_FEATURE="float"
)
space_rhythm_apply_build_policy(space_rhythm_audio_analysis)

install(TARGETS space_rhythm_audio_analysis ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

file(GLOB _space_rhythm_kissfft_release_dlls CONFIGURE_DEPENDS
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/kissfft-float*.dll"
)
file(GLOB _space_rhythm_kissfft_debug_dlls CONFIGURE_DEPENDS
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/kissfft-float*.dll"
)
install(FILES ${_space_rhythm_kissfft_release_dlls}
    DESTINATION bin
    CONFIGURATIONS Release RelWithDebInfo MinSizeRel
)
install(FILES ${_space_rhythm_kissfft_debug_dlls}
    DESTINATION bin
    CONFIGURATIONS Debug
)
install(FILES
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/kissfft/copyright"
    DESTINATION licenses
    RENAME kissfft-BSD-3-Clause.txt
)
