find_package(OpenCV 4.12.0 EXACT REQUIRED COMPONENTS core imgproc video)
if(NOT OpenCV_VERSION VERSION_EQUAL "4.12.0")
    message(FATAL_ERROR "OpenCV 4.12.0 is required exactly; found ${OpenCV_VERSION}")
endif()

add_library(space_rhythm_video_analysis STATIC
    analysis.cpp
)
add_library(SpaceRhythm::VideoAnalysis ALIAS space_rhythm_video_analysis)

target_include_directories(space_rhythm_video_analysis PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(space_rhythm_video_analysis SYSTEM PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(space_rhythm_video_analysis
    PUBLIC
        SpaceRhythm::Core
        SpaceRhythm::MediaAdapter
    PRIVATE
        ${OpenCV_LIBS}
)
target_compile_definitions(space_rhythm_video_analysis PRIVATE
    SPACE_RHYTHM_OPENCV_VERSION="${OpenCV_VERSION}"
)
space_rhythm_apply_build_policy(space_rhythm_video_analysis)

install(TARGETS space_rhythm_video_analysis ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
