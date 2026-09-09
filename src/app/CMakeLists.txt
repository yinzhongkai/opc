qt_add_executable(space_rhythm_app
    main.cpp
)
set_target_properties(space_rhythm_app PROPERTIES
    OUTPUT_NAME "space-rhythm"
    WIN32_EXECUTABLE FALSE
)

qt_add_qml_module(space_rhythm_app
    URI SpaceRhythm
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(space_rhythm_app PRIVATE
    SpaceRhythm::Core
    SpaceRhythm::MediaAdapter
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)
# Qt's generated QML cache sources are third-party generated code and are not
# promoted to /WX. Hand-written libraries and executables remain /WX-clean.
space_rhythm_apply_build_policy(space_rhythm_app ALLOW_GENERATED_WARNINGS)

install(TARGETS space_rhythm_app RUNTIME DESTINATION bin)
