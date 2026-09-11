cmake_minimum_required(VERSION 3.31)

set(SPACE_RHYTHM_TARGET_ARCH "x64" CACHE STRING "Supported target architecture")
set_property(CACHE SPACE_RHYTHM_TARGET_ARCH PROPERTY STRINGS x64)
if(NOT SPACE_RHYTHM_TARGET_ARCH STREQUAL "x64")
    message(FATAL_ERROR
        "Space Rhythm supports only Windows x64; requested architecture: "
        "${SPACE_RHYTHM_TARGET_ARCH}")
endif()

project(SpaceRhythm VERSION 0.1.0 LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Space Rhythm currently supports only Windows x64")
endif()
if(NOT MSVC)
    message(FATAL_ERROR "Space Rhythm requires the MSVC 2022 toolchain")
endif()
if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
    message(FATAL_ERROR "Space Rhythm requires the Ninja generator")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Space Rhythm requires a 64-bit compiler environment")
endif()
if(DEFINED ENV{VSCMD_ARG_TGT_ARCH} AND NOT "$ENV{VSCMD_ARG_TGT_ARCH}" STREQUAL "x64")
    message(FATAL_ERROR "VsDevCmd target architecture must be x64")
endif()
if(NOT MSVC_VERSION EQUAL 1944)
    message(FATAL_ERROR
        "The Qt 6.11.2 SDK ABI baseline requires MSVC 19.44 (MSVC_VERSION=1944); "
        "found ${MSVC_VERSION}")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set(SPACE_RHYTHM_QT_ROOT "" CACHE PATH "Root of the prebuilt Qt 6.11.2 SDK")
if(NOT SPACE_RHYTHM_QT_ROOT)
    message(FATAL_ERROR
        "SPACE_RHYTHM_QT_ROOT must point to the existing Qt 6.11.2 SDK; "
        "this project does not build Qt")
endif()

list(PREPEND CMAKE_PREFIX_PATH "${SPACE_RHYTHM_QT_ROOT}")
list(PREPEND CMAKE_MODULE_PATH
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/ffmpeg")
find_package(FFMPEG REQUIRED)
if(FFMPEG_VERSION)
    set(SPACE_RHYTHM_FFMPEG_VERSION "${FFMPEG_VERSION}" CACHE STRING
        "Resolved FFmpeg package version")
endif()
if(NOT SPACE_RHYTHM_FFMPEG_VERSION)
    message(FATAL_ERROR "The FFmpeg package did not report a version")
endif()
find_package(Qt6 6.11.2 EXACT REQUIRED COMPONENTS
    Core Gui Multimedia Network Qml Quick Test QuickTest)
find_package(GTest CONFIG REQUIRED)

file(REAL_PATH "${SPACE_RHYTHM_QT_ROOT}" _space_rhythm_qt_root)
file(REAL_PATH "${Qt6_DIR}/../../.." _space_rhythm_qt_package_root)
if(NOT _space_rhythm_qt_package_root STREQUAL _space_rhythm_qt_root)
    message(FATAL_ERROR
        "Qt was resolved outside SPACE_RHYTHM_QT_ROOT: ${_space_rhythm_qt_package_root}")
endif()
if(NOT Qt6_VERSION VERSION_EQUAL "6.11.2")
    message(FATAL_ERROR "Qt 6.11.2 is required exactly; found ${Qt6_VERSION}")
endif()

include(CTest)
include(cmake/SpaceRhythmBuild.cmake)
qt_standard_project_setup(REQUIRES 6.11.2)

add_subdirectory(src/domain)
add_subdirectory(src/media)
add_subdirectory(src/audio_analysis)
add_subdirectory(src/audio_render)
add_subdirectory(src/rendering)
add_subdirectory(src/system)
add_subdirectory(src/ui)
add_subdirectory(src/worker)
add_subdirectory(src/app)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

message(STATUS "Space Rhythm target architecture: x64")
message(STATUS "Space Rhythm MSVC baseline: ${MSVC_VERSION}")
message(STATUS "Space Rhythm Qt SDK: ${_space_rhythm_qt_root} (${Qt6_VERSION})")
message(STATUS "Space Rhythm MSVC runtime: dynamic (/MD, /MDd)")
message(STATUS "Space Rhythm FFmpeg: ${SPACE_RHYTHM_FFMPEG_VERSION}")
