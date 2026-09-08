cmake_minimum_required(VERSION 3.22)

project(space_rhythm_qt_smoke LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "The T-012 smoke test is Windows-only.")
endif()

if(NOT MSVC)
    message(FATAL_ERROR "The T-012 baseline requires MSVC 2022.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The T-012 baseline requires an x64 toolchain.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

find_package(
    Qt6 6.11.2 EXACT
    REQUIRED COMPONENTS Core Gui Qml Quick Multimedia
)

qt_standard_project_setup(REQUIRES 6.11.2)

qt_add_executable(space_rhythm_qt_smoke
    main.cpp
)

set_target_properties(space_rhythm_qt_smoke PROPERTIES
    WIN32_EXECUTABLE FALSE
)

qt_add_qml_module(space_rhythm_qt_smoke
    URI SpaceRhythm.BuildSmoke
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_compile_definitions(space_rhythm_qt_smoke PRIVATE
    UNICODE
    _UNICODE
)

target_compile_options(space_rhythm_qt_smoke PRIVATE
    /permissive-
    /W4
)

# Keep handwritten smoke-test code warning-clean without promoting warnings in
# Qt-generated QML cache sources to errors.
set_source_files_properties(main.cpp PROPERTIES COMPILE_OPTIONS /WX)

target_link_libraries(space_rhythm_qt_smoke PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Multimedia
)

install(TARGETS space_rhythm_qt_smoke
    RUNTIME DESTINATION bin
)
