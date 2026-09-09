if(NOT DEFINED APP_EXE OR NOT DEFINED QML_EXE OR NOT DEFINED APP_QML)
    message(FATAL_ERROR "APP_EXE, QML_EXE and APP_QML are required")
endif()

execute_process(
    COMMAND "${APP_EXE}" --smoke
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 20
)
set(_combined "${_stdout}\n${_stderr}")
string(STRIP "${_combined}" _combined_stripped)

if(_result EQUAL 0)
    if(_stdout MATCHES "SPACE_RHYTHM_APP_SMOKE_OK Qt=6.11.2 arch=x64")
        message(STATUS "Generated application QML smoke passed")
        return()
    endif()
    if(NOT "$ENV{SPACE_RHYTHM_ALLOW_WDAC_FALLBACK}" STREQUAL "1" OR
       NOT _combined_stripped STREQUAL "")
        message(FATAL_ERROR "Application exited without the expected smoke marker:\n${_combined}")
    endif()
endif()

# Fall back only for the managed host's exact WDAC rejection. CI and normal
# developer machines must execute the generated application itself.
string(TOLOWER "${_result}" _result_lower)
if(NOT EXISTS "${APP_EXE}")
    message(FATAL_ERROR "Application smoke executable does not exist: ${APP_EXE}")
endif()
if(NOT "$ENV{SPACE_RHYTHM_ALLOW_WDAC_FALLBACK}" STREQUAL "1")
    message(FATAL_ERROR "Application QML smoke failed (${_result}):\n${_combined}")
endif()
if(NOT _result EQUAL 0 AND
   NOT _result_lower MATCHES "(-1058471934|3236495362|0xc0e90002)" AND
   NOT _result_lower STREQUAL "unknown error")
    message(FATAL_ERROR "Application QML smoke failed (${_result}):\n${_combined}")
endif()

execute_process(
    COMMAND "${QML_EXE}" "${APP_QML}" -- --smoke
    RESULT_VARIABLE _fallback_result
    OUTPUT_VARIABLE _fallback_stdout
    ERROR_VARIABLE _fallback_stderr
    TIMEOUT 20
)
if(NOT _fallback_result EQUAL 0)
    message(FATAL_ERROR
        "QML entry fallback failed (${_fallback_result}):\n"
        "${_fallback_stdout}\n${_fallback_stderr}")
endif()

message(WARNING
    "WDAC blocked the generated application or returned no output; "
    "the same Main.qml entry passed with the pinned Qt 6.11.2 qml runtime")
