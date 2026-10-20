# Toy Story 2 (SLUS-00893, USA) native port on the psxport framework.
cmake_minimum_required(VERSION 3.21)

# ccache, when available; must be set before project().
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  message(STATUS "ccache: ${CCACHE_PROGRAM}")
endif()

project(toystory2 CXX C)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# PSXPORT_DIR is the framework source; default external/psxport, linked or cloned by
# psxport's tools/psxport_fetch.py. Override with -DPSXPORT_DIR=/path/to/psxport.
# Derive every framework path from it; never spell `external/psxport` at a call site.
set(PSXPORT_DIR "${CMAKE_SOURCE_DIR}/external/psxport" CACHE PATH
    "psxport framework source directory (default: external/psxport)")
if(NOT EXISTS "${PSXPORT_DIR}/cmake/psxport.cmake")
  message(FATAL_ERROR "PSXPORT_DIR=${PSXPORT_DIR} has no cmake/psxport.cmake — "
                      "run ./run.sh once to fetch it, or point PSXPORT_DIR at a psxport checkout.")
endif()
message(STATUS "psxport framework: ${PSXPORT_DIR}")

# The framework's nested vendors are initialised by psxport's tools/psxport_fetch.py.
if(NOT EXISTS "${PSXPORT_DIR}/vendor/lucent/CMakeLists.txt")
  message(FATAL_ERROR "psxport's vendor/lucent is not checked out in the shared clone — run "
                      "`git -C ${PSXPORT_DIR} submodule update --init vendor/lucent` there once.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Embed psxport's root for its oracle/crt0 tools.
add_subdirectory(${PSXPORT_DIR} ${CMAKE_BINARY_DIR}/psxport_build)

include(cmake/port.cmake)
