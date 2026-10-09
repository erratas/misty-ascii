cmake_minimum_required(VERSION 3.16)
project(misty_ascii VERSION 0.1.0 LANGUAGES CXX)

# Works standalone and as a subproject (add_subdirectory) of the Misty editor.
set(MISTY_ASCII_MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MISTY_ASCII_MAIN_PROJECT ON)
endif()
option(MISTY_ASCII_BUILD_CLI "Build the misty command-line tool" ${MISTY_ASCII_MAIN_PROJECT})
option(MISTY_ASCII_BUILD_TESTS "Build misty-ascii tests" ${MISTY_ASCII_MAIN_PROJECT})

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(misty_ascii STATIC
  src/text.cpp
  src/color.cpp
  src/table.cpp
  src/chart.cpp
)
add_library(misty::ascii ALIAS misty_ascii)
target_include_directories(misty_ascii PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(misty_ascii PUBLIC cxx_std_17)
set_target_properties(misty_ascii PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
  target_compile_options(misty_ascii PRIVATE /W4 /utf-8)
else()
  target_compile_options(misty_ascii PRIVATE -Wall -Wextra)
endif()

if(MISTY_ASCII_BUILD_CLI)
  add_executable(misty cli/main.cpp)
  target_link_libraries(misty PRIVATE misty::ascii)
  if(MSVC)
    target_compile_options(misty PRIVATE /utf-8)
  else()
    target_compile_options(misty PRIVATE -Wall -Wextra)
  endif()
  install(TARGETS misty RUNTIME DESTINATION bin)
endif()

if(MISTY_ASCII_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
