# Unit tests (no framework needed) and golden-file tests of the CLI, compared byte for byte.
add_executable(misty_ascii_unit unit.cpp)
target_link_libraries(misty_ascii_unit PRIVATE misty::ascii)
if(MSVC)
  target_compile_options(misty_ascii_unit PRIVATE /utf-8)
endif()
add_test(NAME unit COMMAND misty_ascii_unit)

# Point the golden tests at another binary (for example the Cosmopolitan build):
#   cmake -DMISTY_ASCII_TEST_BINARY=/path/to/dist/misty ...
set(MISTY_ASCII_TEST_BINARY "" CACHE FILEPATH "misty binary for golden tests (default: built target)")
if(MISTY_ASCII_TEST_BINARY)
  set(misty_under_test "${MISTY_ASCII_TEST_BINARY}")
elseif(TARGET misty)
  set(misty_under_test "$<TARGET_FILE:misty>")
endif()

if(misty_under_test)
  # Each line of golden/cases.txt: name | input file (or -) | expected exit | args...
  # Expected stdout lives in golden/<name>.out. Regenerate with MISTY_UPDATE_GOLDEN=1 ctest.
  file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/golden/cases.txt cases ENCODING UTF-8)
  foreach(line IN LISTS cases)
    if(line MATCHES "^#" OR line STREQUAL "")
      continue()
    endif()
    string(REPLACE "|" ";" fields "${line}")
    list(GET fields 0 name)
    list(GET fields 1 input)
    list(GET fields 2 code)
    list(LENGTH fields n)
    set(args "")
    if(n GREATER 3)
      list(GET fields 3 args)
    endif()
    string(STRIP "${name}" name)
    string(STRIP "${input}" input)
    string(STRIP "${code}" code)
    string(STRIP "${args}" args)
    add_test(NAME golden_${name}
      COMMAND ${CMAKE_COMMAND}
        -DMISTY=${misty_under_test}
        -DARGS=${args}
        -DINPUT=${input}
        -DCODE=${code}
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/golden/${name}.out
        -DDATA=${CMAKE_CURRENT_SOURCE_DIR}/data
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_golden.cmake)
  endforeach()
endif()
