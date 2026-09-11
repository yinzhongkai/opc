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
    LABELS "t021;golden;media;headless;gate-g1;gate-g3"
    TIMEOUT 180
)

add_test(
    NAME audio.golden_generate
    COMMAND "${SPACE_RHYTHM_POWERSHELL_EXECUTABLE}" -NoProfile -File
        "${CMAKE_CURRENT_SOURCE_DIR}/golden/audio/Generate-AudioDspVectors.ps1"
)
set_tests_properties(audio.golden_generate PROPERTIES
    FIXTURES_SETUP golden-audio
    LABELS "t031;t032;golden;audio;headless;gate-g1;gate-g3"
    TIMEOUT 60
)

add_executable(space_rhythm_audio_render_tests
    unit/audio_render_test.cpp
)
target_link_libraries(space_rhythm_audio_render_tests PRIVATE
    SpaceRhythm::AudioRender
    SpaceRhythm::AudioPreviewQt
    GTest::gtest_main
    Qt6::Multimedia
)
target_compile_definitions(space_rhythm_audio_render_tests PRIVATE
    SPACE_RHYTHM_GOLDEN_AUDIO_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/audio/generated"
)
space_rhythm_apply_build_policy(space_rhythm_audio_render_tests)
gtest_add_tests(
    TARGET space_rhythm_audio_render_tests
    TEST_LIST space_rhythm_audio_render_test_cases
)
set_tests_properties(${space_rhythm_audio_render_test_cases} PROPERTIES
    FIXTURES_REQUIRED golden-audio
    LABELS "t032;unit;golden;audio;headless;gate-g1;gate-g3"
)

add_executable(space_rhythm_audio_render_benchmark
    performance/audio_render_benchmark.cpp
)
target_link_libraries(space_rhythm_audio_render_benchmark PRIVATE
    SpaceRhythm::AudioRender
    Psapi
)
target_compile_definitions(space_rhythm_audio_render_benchmark PRIVATE
    SPACE_RHYTHM_GOLDEN_AUDIO_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/audio/generated"
)
space_rhythm_apply_build_policy(space_rhythm_audio_render_benchmark)
add_test(
    NAME audio.render_performance_smoke
    COMMAND space_rhythm_audio_render_benchmark --smoke
)
set_tests_properties(audio.render_performance_smoke PROPERTIES
    FIXTURES_REQUIRED golden-audio
    LABELS "t032;performance;audio;headless;measured;not-evaluated"
    TIMEOUT 60
)

add_executable(space_rhythm_audio_analysis_tests
    unit/audio_analysis_test.cpp
)
target_link_libraries(space_rhythm_audio_analysis_tests PRIVATE
    SpaceRhythm::AudioAnalysis
    GTest::gtest_main
)
target_compile_definitions(space_rhythm_audio_analysis_tests PRIVATE
    SPACE_RHYTHM_GOLDEN_AUDIO_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/audio/generated"
    SPACE_RHYTHM_GOLDEN_MEDIA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/media/generated"
)
space_rhythm_apply_build_policy(space_rhythm_audio_analysis_tests)
gtest_add_tests(
    TARGET space_rhythm_audio_analysis_tests
    TEST_LIST space_rhythm_audio_analysis_test_cases
)
set_tests_properties(${space_rhythm_audio_analysis_test_cases} PROPERTIES
    FIXTURES_REQUIRED "golden-audio;golden-media"
    LABELS "t031;unit;golden;audio;headless;gate-g1;gate-g3"
)

add_executable(space_rhythm_audio_analysis_benchmark
    performance/audio_analysis_benchmark.cpp
)
target_link_libraries(space_rhythm_audio_analysis_benchmark PRIVATE
    SpaceRhythm::AudioAnalysis
    Psapi
)
target_compile_definitions(space_rhythm_audio_analysis_benchmark PRIVATE
    SPACE_RHYTHM_GOLDEN_AUDIO_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/audio/generated"
)
space_rhythm_apply_build_policy(space_rhythm_audio_analysis_benchmark)

add_test(
    NAME contract.golden.media_audit
    COMMAND "${SPACE_RHYTHM_POWERSHELL_EXECUTABLE}" -NoProfile -File
        "${CMAKE_CURRENT_SOURCE_DIR}/contract/Test-GoldenMediaAudit.ps1"
        -ManifestPath "${CMAKE_CURRENT_SOURCE_DIR}/golden/media/fixtures-v1.json"
        -ActualEvidencePath "${CMAKE_CURRENT_SOURCE_DIR}/golden/media/generated/actual-hashes-and-probe-v1.json"
        -FixtureRoot "${CMAKE_CURRENT_SOURCE_DIR}/golden/media/generated"
        -PolicyPath "${CMAKE_CURRENT_SOURCE_DIR}/contract/golden-media-audit-v1.json"
        -LicensePath "${CMAKE_CURRENT_SOURCE_DIR}/golden/media/LICENSE.md"
)
set_tests_properties(contract.golden.media_audit PROPERTIES
    FIXTURES_REQUIRED golden-media
    LABELS "t021;contract;golden;media;headless;gate-g1;gate-g3;gate-g4"
)

add_executable(space_rhythm_media_tests
    unit/media_golden_test.cpp
)
target_link_libraries(space_rhythm_media_tests PRIVATE
    SpaceRhythm::MediaAdapter
    GTest::gtest_main
)
target_include_directories(space_rhythm_media_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/src/media"
)
target_include_directories(space_rhythm_media_tests SYSTEM PRIVATE
    ${FFMPEG_INCLUDE_DIRS}
)
target_compile_definitions(space_rhythm_media_tests PRIVATE
    SPACE_RHYTHM_GOLDEN_MEDIA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/media/generated"
)
space_rhythm_apply_build_policy(space_rhythm_media_tests)
gtest_add_tests(TARGET space_rhythm_media_tests TEST_LIST space_rhythm_media_test_cases)
set_tests_properties(${space_rhythm_media_test_cases} PROPERTIES
    FIXTURES_REQUIRED golden-media
    LABELS "t021;golden;media;unit;headless;gate-g1;gate-g3"
)

add_executable(space_rhythm_contract_tests
    contract/core_public_contract_test.cpp
    contract/system_public_contract_test.cpp
)
target_link_libraries(space_rhythm_contract_tests PRIVATE
    SpaceRhythm::Core
    SpaceRhythm::System
    GTest::gtest_main
    Qt6::Core
)
space_rhythm_apply_build_policy(space_rhythm_contract_tests)
gtest_add_tests(TARGET space_rhythm_contract_tests TEST_LIST space_rhythm_contract_test_cases)
set_tests_properties(${space_rhythm_contract_test_cases} PROPERTIES
    LABELS "t021;contract;headless;gate-g1;gate-g2;gate-g4"
)

add_executable(space_rhythm_media_contract_tests
    contract/media_public_contract_test.cpp
)
target_link_libraries(space_rhythm_media_contract_tests PRIVATE
    SpaceRhythm::MediaAdapter
    GTest::gtest_main
)
target_compile_definitions(space_rhythm_media_contract_tests PRIVATE
    SPACE_RHYTHM_GOLDEN_MEDIA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/media/generated"
)
space_rhythm_apply_build_policy(space_rhythm_media_contract_tests)
gtest_add_tests(TARGET space_rhythm_media_contract_tests TEST_LIST space_rhythm_media_contract_test_cases)
set_tests_properties(${space_rhythm_media_contract_test_cases} PROPERTIES
    FIXTURES_REQUIRED golden-media
    LABELS "t021;contract;golden;media;headless;gate-g1;gate-g2;gate-g3;gate-g4"
)

add_executable(space_rhythm_render_contract_tests
    contract/render_public_contract_test.cpp
)
target_link_libraries(space_rhythm_render_contract_tests PRIVATE
    SpaceRhythm::RenderingContract
    GTest::gtest_main
)
space_rhythm_apply_build_policy(space_rhythm_render_contract_tests)
gtest_add_tests(
    TARGET space_rhythm_render_contract_tests
    TEST_LIST space_rhythm_render_contract_test_cases
)
set_tests_properties(${space_rhythm_render_contract_test_cases} PROPERTIES
    LABELS "t033;contract;rendering;headless;gate-g2;gate-g3;gate-g4"
)

add_executable(space_rhythm_render_geometry_tests
    unit/render_geometry_test.cpp
)
target_link_libraries(space_rhythm_render_geometry_tests PRIVATE
    SpaceRhythm::RenderingGeometry
    GTest::gtest_main
)
space_rhythm_apply_build_policy(space_rhythm_render_geometry_tests)
gtest_add_tests(
    TARGET space_rhythm_render_geometry_tests
    TEST_LIST space_rhythm_render_geometry_test_cases
)
set_tests_properties(${space_rhythm_render_geometry_test_cases} PROPERTIES
    LABELS "t034;unit;rendering;geometry-core;headless;gate-g1;gate-g3"
)

add_executable(space_rhythm_render_scene_graph_tests
    unit/render_scene_graph_test.cpp
)
target_link_libraries(space_rhythm_render_scene_graph_tests PRIVATE
    SpaceRhythm::SceneGraph
    GTest::gtest_main
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)
space_rhythm_apply_build_policy(space_rhythm_render_scene_graph_tests)
gtest_add_tests(
    TARGET space_rhythm_render_scene_graph_tests
    TEST_LIST space_rhythm_render_scene_graph_test_cases
)
set_tests_properties(${space_rhythm_render_scene_graph_test_cases} PROPERTIES
    LABELS "t034;unit;rendering;scene-graph;headless;gate-g1;gate-g3"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${SPACE_RHYTHM_QT_ROOT}/bin"
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
gtest_add_tests(TARGET space_rhythm_core_tests TEST_LIST space_rhythm_core_test_cases)
set_tests_properties(${space_rhythm_core_test_cases} PROPERTIES
    LABELS "t021;unit;headless;gate-g0;gate-g1"
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
set_tests_properties(qt.core_smoke PROPERTIES
    LABELS "t021;qt;headless;smoke;gate-g0;gate-g1"
)

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
    LABELS "t021;qml;qt;headless;smoke;gate-g0;gate-g1"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_add_executable(space_rhythm_ui_bridge_tests
    qt/ui_bridge_test.cpp
)
target_link_libraries(space_rhythm_ui_bridge_tests PRIVATE
    SpaceRhythm::UiBridge
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
)
space_rhythm_apply_build_policy(space_rhythm_ui_bridge_tests ALLOW_GENERATED_WARNINGS)
add_test(NAME qt.ui_bridge COMMAND space_rhythm_ui_bridge_tests)
set_tests_properties(qt.ui_bridge PROPERTIES
    LABELS "t025;qt;bridge;headless;gate-g0;gate-g1"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Basic"
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${SPACE_RHYTHM_QT_ROOT}/bin"
)

qt_add_executable(space_rhythm_ui_qml_tests
    qml/quick_test_main.cpp
)
target_compile_definitions(space_rhythm_ui_qml_tests PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml/t025"
)
target_link_libraries(space_rhythm_ui_qml_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickTest
)
space_rhythm_apply_build_policy(space_rhythm_ui_qml_tests ALLOW_GENERATED_WARNINGS)
add_test(NAME qml.ui_shell COMMAND space_rhythm_ui_qml_tests)
set_tests_properties(qml.ui_shell PROPERTIES
    LABELS "t025;qml;qt;headless;gate-g0;gate-g1"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Basic"
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${SPACE_RHYTHM_QT_ROOT}/bin"
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
    LABELS "t021;t025;app;integration;qml;qt;headless;smoke;gate-g0"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Basic"
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${SPACE_RHYTHM_QT_ROOT}/bin"
)

add_test(
    NAME worker.process_smoke
    COMMAND "${CMAKE_COMMAND}"
        -DWORKER_EXE=$<TARGET_FILE:space_rhythm_worker>
        -DQML_TEST_RUNNER=${SPACE_RHYTHM_QT_ROOT}/bin/qmltestrunner.exe
        -DQML_TEST_DIR=${CMAKE_CURRENT_SOURCE_DIR}/qml
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunWorkerSmoke.cmake"
)
set_tests_properties(worker.process_smoke PROPERTIES
    LABELS "t021;worker;integration;qt;headless;smoke;gate-g0;gate-g4"
)

foreach(_unsupported_arch IN ITEMS x86 arm64)
    add_test(
        NAME build.reject_${_unsupported_arch}
        COMMAND "${CMAKE_COMMAND}"
            -DARCH=${_unsupported_arch}
            -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/reject-${_unsupported_arch}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RejectArchitecture.cmake"
    )
    set_tests_properties(build.reject_${_unsupported_arch} PROPERTIES
        LABELS "t021;compatibility;headless;build-guard;gate-g0"
    )
endforeach()
