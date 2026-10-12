# Runs ortha with one argument and checks exit code, stdout and stderr separately
# (a plain CTest with PASS_REGULAR_EXPRESSION ignores the exit code and mixes both streams).
# Usage: cmake -DORTHA=<exe> -DARG=<arg> -DRC=<code> -DSTDOUT=<regex> -DSTDERR=<regex> -P run_ortha.cmake
execute_process(COMMAND ${ORTHA} ${ARG} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL RC)
    message(FATAL_ERROR "ortha ${ARG}: exit code ${rc}, expected ${RC}\nstdout: ${out}\nstderr: ${err}")
endif()
if(NOT out MATCHES "${STDOUT}")
    message(FATAL_ERROR "ortha ${ARG}: stdout '${out}' does not match '${STDOUT}'")
endif()
if(NOT err MATCHES "${STDERR}")
    message(FATAL_ERROR "ortha ${ARG}: stderr '${err}' does not match '${STDERR}'")
endif()
