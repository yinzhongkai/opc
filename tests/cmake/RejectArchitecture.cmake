if(NOT DEFINED ARCH OR NOT DEFINED SOURCE_DIR OR NOT DEFINED BINARY_DIR)
    message(FATAL_ERROR "ARCH, SOURCE_DIR and BINARY_DIR are required")
endif()

file(REMOVE_RECURSE "${BINARY_DIR}")
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${SOURCE_DIR}"
        -B "${BINARY_DIR}"
        -G Ninja
        -DSPACE_RHYTHM_TARGET_ARCH=${ARCH}
        -DVCPKG_MANIFEST_MODE=OFF
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
)

set(_combined "${_stdout}\n${_stderr}")
if(_result EQUAL 0)
    message(FATAL_ERROR "Unsupported architecture ${ARCH} configured successfully")
endif()
if(NOT _combined MATCHES "supports only Windows x64")
    message(FATAL_ERROR
        "${ARCH} failed for an unexpected reason. Configure output:\n${_combined}")
endif()

message(STATUS "Confirmed that ${ARCH} is rejected by the Windows x64 guard")
