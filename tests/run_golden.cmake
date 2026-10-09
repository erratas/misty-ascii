# Runs one golden case: MISTY ARGS (space separated) with stdin from DATA/INPUT, checks the
# exit code equals CODE and stdout equals EXPECTED byte for byte.
separate_arguments(arglist UNIX_COMMAND "${ARGS}")
set(in_args "")
if(NOT INPUT STREQUAL "-")
  set(in_args INPUT_FILE "${DATA}/${INPUT}")
endif()
# Fixed environment so results never depend on the machine running the tests.
set(ENV{COLUMNS} 80)
unset(ENV{NO_COLOR})
unset(ENV{FORCE_COLOR})
unset(ENV{COLORTERM})
execute_process(
  COMMAND ${LAUNCHER} "${MISTY}" ${arglist}
  ${in_args}
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
  RESULT_VARIABLE rc)

if(DEFINED ENV{MISTY_UPDATE_GOLDEN})
  file(WRITE "${EXPECTED}" "${out}")
  message(STATUS "updated ${EXPECTED}")
endif()

if(NOT rc STREQUAL CODE)
  message(FATAL_ERROR "exit code ${rc}, expected ${CODE}\nstderr: ${err}\nstdout:\n${out}")
endif()
if(NOT EXISTS "${EXPECTED}")
  message(FATAL_ERROR "missing golden file ${EXPECTED}")
endif()
file(READ "${EXPECTED}" want)
if(NOT out STREQUAL want)
  message(FATAL_ERROR "output differs from ${EXPECTED}\n--- got ---\n${out}\n--- want ---\n${want}")
endif()
