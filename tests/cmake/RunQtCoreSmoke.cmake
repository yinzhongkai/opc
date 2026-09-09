if(NOT DEFINED PRIMARY_EXE OR NOT DEFINED FALLBACK_EXE OR NOT DEFINED FALLBACK_INPUT)
    message(FATAL_ERROR "PRIMARY_EXE, FALLBACK_EXE and FALLBACK_INPUT are required")
endif()

execute_process(
    COMMAND "${PRIMARY_EXE}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 20
)
set(_combined "${_stdout}\n${_stderr}")
string(STRIP "${_combined}" _combined_stripped)

if(_result EQUAL 0)
    if(NOT _combined_stripped STREQUAL "")
        message(STATUS "Generated Qt test smoke passed")
        return()
    endif()
    if(NOT "$ENV{SPACE_RHYTHM_ALLOW_WDAC_FALLBACK}" STREQUAL "1")
        message(FATAL_ERROR "Qt test exited without producing output")
    endif()
endif()

# 0xC0E90002 is the Windows application-control policy rejection observed on
# the managed development host. Do not hide any other test failure.
if(NOT "$ENV{SPACE_RHYTHM_ALLOW_WDAC_FALLBACK}" STREQUAL "1")
    message(FATAL_ERROR "Qt Test smoke failed (${_result}):\n${_combined}")
endif()
string(TOLOWER "${_result}" _result_lower)
if(NOT _result EQUAL 0 AND
   NOT _result_lower MATCHES "(-1058471934|3236495362|0xc0e90002)" AND
   NOT _result_lower STREQUAL "unknown error")
    message(FATAL_ERROR "Qt Test smoke failed (${_result}):\n${_combined}")
endif()

execute_process(
    COMMAND "${FALLBACK_EXE}" -input "${FALLBACK_INPUT}"
    RESULT_VARIABLE _fallback_result
    OUTPUT_VARIABLE _fallback_stdout
    ERROR_VARIABLE _fallback_stderr
    TIMEOUT 20
)
if(NOT _fallback_result EQUAL 0)
    message(FATAL_ERROR
        "Qt Core fallback failed (${_fallback_result}):\n"
        "${_fallback_stdout}\n${_fallback_stderr}")
endif()

message(WARNING
    "WDAC blocked the generated Qt test executable or returned no output; "
    "the pinned Qt 6.11.2 qmltestrunner independently passed")
