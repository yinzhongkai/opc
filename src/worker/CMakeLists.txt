qt_add_executable(space_rhythm_worker
    main.cpp
)
set_target_properties(space_rhythm_worker PROPERTIES
    OUTPUT_NAME "space-rhythm-worker"
    WIN32_EXECUTABLE FALSE
)
target_link_libraries(space_rhythm_worker PRIVATE
    SpaceRhythm::Core
    SpaceRhythm::MediaAdapter
    Qt6::Core
    Qt6::Network
)
space_rhythm_apply_build_policy(space_rhythm_worker)

install(TARGETS space_rhythm_worker RUNTIME DESTINATION bin)
