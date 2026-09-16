add_library(space_rhythm_system STATIC
    assets_cache.cpp
    ipc.cpp
    job_coordinator.cpp
    project_store.cpp
)
add_library(SpaceRhythm::System ALIAS space_rhythm_system)

target_include_directories(space_rhythm_system PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(space_rhythm_system
    PUBLIC SpaceRhythm::Core
    PRIVATE Qt6::Core Qt6::Network
)
space_rhythm_apply_build_policy(space_rhythm_system)

install(TARGETS space_rhythm_system ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
install(FILES schema/space-rhythm-project-v2.schema.json DESTINATION share/space-rhythm/schema)
