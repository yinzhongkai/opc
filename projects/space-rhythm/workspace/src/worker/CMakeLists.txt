add_library(space_rhythm_ui_worker_protocol STATIC
    include/space_rhythm/worker/ui_worker_protocol.hpp
    ui_worker_protocol.cpp
)
add_library(SpaceRhythm::UiWorkerProtocol ALIAS space_rhythm_ui_worker_protocol)
target_include_directories(space_rhythm_ui_worker_protocol PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(space_rhythm_ui_worker_protocol PUBLIC
    SpaceRhythm::Core
    SpaceRhythm::MediaAdapter
    SpaceRhythm::AudioAnalysis
    SpaceRhythm::System
    Qt6::Core
)
space_rhythm_apply_build_policy(space_rhythm_ui_worker_protocol)

qt_add_executable(space_rhythm_worker
    main.cpp
    ui_worker_server.cpp
)
set_target_properties(space_rhythm_worker PROPERTIES
    OUTPUT_NAME "space-rhythm-worker"
    WIN32_EXECUTABLE FALSE
)
target_link_libraries(space_rhythm_worker PRIVATE
    SpaceRhythm::Core
    SpaceRhythm::MediaAdapter
    SpaceRhythm::AudioAnalysis
    SpaceRhythm::System
    SpaceRhythm::UiWorkerProtocol
    Qt6::Core
    Qt6::Network
)
space_rhythm_apply_build_policy(space_rhythm_worker)

install(TARGETS space_rhythm_worker RUNTIME DESTINATION bin)
install(TARGETS space_rhythm_ui_worker_protocol ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
