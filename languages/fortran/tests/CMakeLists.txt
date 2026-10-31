# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds a Model.
add_executable(fortran_grammar_test grammar_test.cpp)
target_link_libraries(fortran_grammar_test PRIVATE fortranCore cursiv::test)
target_compile_definitions(fortran_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME fortran_grammar_test COMMAND fortran_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET fortran_cli)
  add_test(NAME fortran_cli_test COMMAND fortran_cli
           ${PROJECT_SOURCE_DIR}/example/4105.f90)
endif()
