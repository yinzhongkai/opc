if(NOT DEFINED PRIMARY_EXE OR NOT DEFINED REPORT_STEM)
    message(FATAL_ERROR "PRIMARY_EXE and REPORT_STEM are required")
endif()

set(_report "$ENV{TEMP}/${REPORT_STEM}.txt")
file(REMOVE "${_report}")
set(_test_arguments)
if(DEFINED TEST_FUNCTION AND NOT TEST_FUNCTION STREQUAL "")
    list(APPEND _test_arguments "${TEST_FUNCTION}")
endif()
list(APPEND _test_arguments -o "${_report},txt")
execute_process(
    COMMAND "${PRIMARY_EXE}" ${_test_arguments}
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 180
)

set(_report_text "")
if(EXISTS "${_report}")
    file(READ "${_report}" _report_text)
endif()
string(STRIP "${_report_text}" _report_stripped)

if(NOT _result EQUAL 0)
    message(FATAL_ERROR
        "Qt Test failed (${_result}):\n${_report_text}\n${_stdout}\n${_stderr}")
endif()
if(_report_stripped STREQUAL "")
    message(FATAL_ERROR "Qt Test exited successfully without producing its report")
endif()

message(STATUS "Qt Test passed:\n${_report_text}")
