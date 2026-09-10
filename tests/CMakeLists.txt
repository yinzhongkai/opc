include(GoogleTest)

find_program(SPACE_RHYTHM_POWERSHELL_EXECUTABLE NAMES pwsh powershell REQUIRED)
set(SPACE_RHYTHM_FFMPEG_TOOL_DIR
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/ffmpeg")
add_test(
    NAME media.golden_generate
    COMMAND "${SPACE_RHYTHM_POWERSHELL_EXECUTABLE}" -NoProfile -File
        "${CMAKE_CURRENT_SOURCE_DIR}/golden/media/Generate-GoldenMedia.ps1"
        -FfmpegPath "${SPACE_RHYTHM_FFMPEG_TOOL_DIR}/ffmpeg.exe"
        -FfprobePath "${SPACE_RHYTHM_FFMPEG_TOOL_DIR}/ffprobe.exe"
)
set_tests_properties(media.golden_generate PROPERTIES
    FIXTURES_SETUP golden-media
    LABELS "golden;media"
    TIMEOUT 180
)

add_executable(space_rhythm_media_tests
    unit/media_golden_test.cpp
)
target_link_libraries(space_rhythm_media_tests PRIVATE
    SpaceRhythm::MediaAdapter
    GTest::gtest_main
)
target_compile_definitions(space_rhythm_media_tests PRIVATE
    SPACE_RHYTHM_GOLDEN_MEDIA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/media/generated"
)
space_rhythm_apply_build_policy(space_rhythm_media_tests)
gtest_discover_tests(space_rhythm_media_tests
    DISCOVERY_MODE PRE_TEST
    DISCOVERY_TIMEOUT 120
    PROPERTIES
        FIXTURES_REQUIRED golden-media
        LABELS "golden;media;unit"
)

add_executable(space_rhythm_core_tests
    unit/core_contract_vectors_test.cpp
    unit/core_skeleton_test.cpp
    unit/ipc_system_test.cpp
    unit/job_system_test.cpp
    unit/project_store_test.cpp
)
target_link_libraries(space_rhythm_core_tests PRIVATE
    SpaceRhythm::Core
    SpaceRhythm::MediaAdapter
    SpaceRhythm::System
    GTest::gtest_main
    Qt6::Core
    Qt6::Network
)
add_dependencies(space_rhythm_core_tests space_rhythm_worker)
target_compile_definitions(space_rhythm_core_tests PRIVATE
    SPACE_RHYTHM_WORKER_EXE="$<TARGET_FILE:space_rhythm_worker>"
)
space_rhythm_apply_build_policy(space_rhythm_core_tests)
gtest_discover_tests(space_rhythm_core_tests
    DISCOVERY_MODE PRE_TEST
    PROPERTIES LABELS "unit"
)

qt_add_executable(space_rhythm_qt_smoke
    qt/qt_core_smoke.cpp
)
target_link_libraries(space_rhythm_qt_smoke PRIVATE
    SpaceRhythm::Core
    Qt6::Core
    Qt6::Test
)
space_rhythm_apply_build_policy(space_rhythm_qt_smoke ALLOW_GENERATED_WARNINGS)
add_test(
    NAME qt.core_smoke
    COMMAND "${CMAKE_COMMAND}"
        -DPRIMARY_EXE=$<TARGET_FILE:space_rhythm_qt_smoke>
        -DFALLBACK_EXE=${SPACE_RHYTHM_QT_ROOT}/bin/qmltestrunner.exe
        -DFALLBACK_INPUT=${CMAKE_CURRENT_SOURCE_DIR}/qml
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunQtCoreSmoke.cmake"
)
set_tests_properties(qt.core_smoke PROPERTIES LABELS "qt;smoke")

qt_add_executable(space_rhythm_qml_smoke
    qml/quick_test_main.cpp
)
target_compile_definitions(space_rhythm_qml_smoke PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml"
)
target_link_libraries(space_rhythm_qml_smoke PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickTest
)
space_rhythm_apply_build_policy(space_rhythm_qml_smoke ALLOW_GENERATED_WARNINGS)
add_test(
    NAME qml.quick_smoke
    COMMAND "${CMAKE_COMMAND}"
        -DPRIMARY_EXE=$<TARGET_FILE:space_rhythm_qml_smoke>
        -DFALLBACK_EXE=${SPACE_RHYTHM_QT_ROOT}/bin/qmltestrunner.exe
        -DFALLBACK_INPUT=${CMAKE_CURRENT_SOURCE_DIR}/qml
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunQtCoreSmoke.cmake"
)
set_tests_properties(qml.quick_smoke PROPERTIES
    LABELS "qml;qt;smoke"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_test(
    NAME app.qml_smoke
    COMMAND "${CMAKE_COMMAND}"
        -DAPP_EXE=$<TARGET_FILE:space_rhythm_app>
        -DQML_EXE=${SPACE_RHYTHM_QT_ROOT}/bin/qml.exe
        -DAPP_QML=${PROJECT_SOURCE_DIR}/src/app/qml/Main.qml
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunAppSmoke.cmake"
)
set_tests_properties(app.qml_smoke PROPERTIES
    LABELS "app;qml;qt;smoke"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_test(
    NAME worker.process_smoke
    COMMAND "${CMAKE_COMMAND}"
        -DWORKER_EXE=$<TARGET_FILE:space_rhythm_worker>
        -DQML_TEST_RUNNER=${SPACE_RHYTHM_QT_ROOT}/bin/qmltestrunner.exe
        -DQML_TEST_DIR=${CMAKE_CURRENT_SOURCE_DIR}/qml
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunWorkerSmoke.cmake"
)
set_tests_properties(worker.process_smoke PROPERTIES LABELS "worker;qt;smoke")

foreach(_unsupported_arch IN ITEMS x86 arm64)
    add_test(
        NAME build.reject_${_unsupported_arch}
        COMMAND "${CMAKE_COMMAND}"
            -DARCH=${_unsupported_arch}
            -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/reject-${_unsupported_arch}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RejectArchitecture.cmake"
    )
    set_tests_properties(build.reject_${_unsupported_arch} PROPERTIES LABELS "build-guard")
endforeach()
