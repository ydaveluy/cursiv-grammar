cmake_minimum_required(VERSION 3.30)
project(cobol85 LANGUAGES CXX)

# COBOL's reserved-word list alone is >500 tokens, past DynSet's default
# 8-word (512-bit) capacity. This must reach cursiv::core itself (added
# below), not just this project's own targets: DynSet's layout has to agree
# across every translation unit, or the two disagree on the same struct.
add_compile_definitions(CURSIV_SET_WORDS=32u)

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
