cmake_minimum_required(VERSION 3.30)
project(sysmlv2 LANGUAGES CXX)

include(FetchContent)
# A local checkout wins over the tag: `-DCURSIV_SOURCE_DIR=<path>` is how you
# build this language against an engine you are also editing.
if(CURSIV_SOURCE_DIR)
  FetchContent_Declare(cursiv SOURCE_DIR ${CURSIV_SOURCE_DIR})
else()
  FetchContent_Declare(cursiv
      GIT_REPOSITORY https://github.com/ydaveluy/cursiv.git
      GIT_TAG        main)
endif()
FetchContent_MakeAvailable(cursiv)

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)
