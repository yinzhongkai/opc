if(NOT DEFINED WORKER_EXE OR NOT DEFINED QML_TEST_RUNNER OR NOT DEFINED QML_TEST_DIR)
    message(FATAL_ERROR "WORKER_EXE, QML_TEST_RUNNER and QML_TEST_DIR are required")
endif()

execute_process(
    COMMAND "${WORKER_EXE}" --smoke
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 20
)
set(_combined "${_stdout}\n${_stderr}")
string(STRIP "${_combined}" _combined_stripped)

if(_result EQUAL 0)
    if(_stdout MATCHES "SPACE_RHYTHM_WORKER_SMOKE_OK Qt=6.11.2 arch=x64")
        message(STATUS "Generated worker smoke passed")
        return()
    endif()
    if(NOT "$ENV{SPACE_RHYTHM_ALLOW_WDAC_FALLBACK}" STREQUAL "1")
        message(FATAL_ERROR "Worker exited without the expected smoke marker:\n${_combined}")
    endif()
    if(NOT _combined_stripped STREQUAL "")
        message(FATAL_ERROR "Worker produced unexpected smoke output:\n${_combined}")
    endif()
endif()

if(NOT EXISTS "${WORKER_EXE}")
    message(FATAL_ERROR "Worker smoke executable does not exist: ${WORKER_EXE}")
endif()
string(TOLOWER "${_result}" _result_lower)
if(NOT "$ENV{SPACE_RHYTHM_ALLOW_WDAC_FALLBACK}" STREQUAL "1")
    message(FATAL_ERROR "Worker smoke failed (${_result}):\n${_combined}")
endif()
if(NOT _result EQUAL 0 AND
   NOT _result_lower MATCHES "(-1058471934|3236495362|0xc0e90002)" AND
   NOT _result_lower STREQUAL "unknown error")
    message(FATAL_ERROR "Worker smoke failed (${_result}):\n${_combined}")
endif()

execute_process(
    COMMAND "${QML_TEST_RUNNER}" -input "${QML_TEST_DIR}"
    RESULT_VARIABLE _fallback_result
    OUTPUT_VARIABLE _fallback_stdout
    ERROR_VARIABLE _fallback_stderr
    TIMEOUT 20
)
if(NOT _fallback_result EQUAL 0)
    message(FATAL_ERROR
        "Qt runtime fallback failed (${_fallback_result}):\n"
        "${_fallback_stdout}\n${_fallback_stderr}")
endif()

message(WARNING
    "WDAC fallback was explicitly enabled after the generated worker process "
    "was blocked or returned no output; the pinned Qt 6.11.2 runtime "
    "independently passed the QML smoke")
