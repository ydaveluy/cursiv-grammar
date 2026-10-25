# Every port in one build tree. cursiv is declared here, before any port: the
# first FetchContent_Declare of a name wins, so each port's own declaration
# finds it populated and the ports share one engine.
cmake_minimum_required(VERSION 3.30)
project(cursiv_grammars LANGUAGES CXX)

include(FetchContent)
if(CURSIV_SOURCE_DIR)
  FetchContent_Declare(cursiv SOURCE_DIR ${CURSIV_SOURCE_DIR})
else()
  FetchContent_Declare(cursiv
      GIT_REPOSITORY https://github.com/ydaveluy/cursiv.git
      GIT_TAG        main)
endif()
FetchContent_MakeAvailable(cursiv)

find_program(CURSIV_GRAMMARS_CCACHE ccache)
if(CURSIV_GRAMMARS_CCACHE AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CURSIV_GRAMMARS_CCACHE})
endif()
# No port has an `import`: the module scan would only add a preprocessing pass
# to every unit of every port.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Here and not only in the ports: without it at the top, ctest run from the
# build root finds none of their tests.
enable_testing()

set(CURSIV_GRAMMARS "" CACHE STRING
    "Ports to build, by directory name under languages/; empty builds every port but the excluded")
set(CURSIV_GRAMMARS_EXCLUDE "" CACHE STRING
    "Ports left out when CURSIV_GRAMMARS is empty")

set(_languages ${CMAKE_CURRENT_SOURCE_DIR}/languages)
file(GLOB _entries LIST_DIRECTORIES true CONFIGURE_DEPENDS
     RELATIVE ${_languages} ${_languages}/*)
set(_ports "")
foreach(_entry IN LISTS _entries)
  if(EXISTS ${_languages}/${_entry}/CMakeLists.txt AND
     IS_DIRECTORY ${_languages}/${_entry}/src)
    list(APPEND _ports ${_entry})
  endif()
endforeach()

foreach(_port IN LISTS CURSIV_GRAMMARS CURSIV_GRAMMARS_EXCLUDE)
  if(NOT _port IN_LIST _ports)
    message(FATAL_ERROR "cursiv_grammars: no port named `${_port}`")
  endif()
endforeach()

if(CURSIV_GRAMMARS)
  set(_selected ${CURSIV_GRAMMARS})
else()
  set(_selected ${_ports})
  if(CURSIV_GRAMMARS_EXCLUDE)
    list(REMOVE_ITEM _selected ${CURSIV_GRAMMARS_EXCLUDE})
  endif()
endif()

foreach(_port IN LISTS _selected)
  add_subdirectory(languages/${_port})
endforeach()
list(LENGTH _selected _count)
list(LENGTH _ports _total)
message(STATUS "cursiv_grammars: ${_count} of ${_total} ports")

# Run only the benchmarks already built; `bench` remains the build target.
add_custom_target(run-bench
    COMMAND ${CMAKE_COMMAND}
        "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
        "-DCONFIG=$<CONFIG>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/bench/run.cmake"
    USES_TERMINAL
    VERBATIM)
