# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: the sample parses, and the tree holds what it declares.
add_executable(xyz_grammar_test grammar_test.cpp)
target_link_libraries(xyz_grammar_test PRIVATE xyzCore cursiv::test)
target_compile_definitions(xyz_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME xyz_grammar_test COMMAND xyz_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET xyz_cli)
  add_test(NAME xyz_cli_test COMMAND xyz_cli
           ${PROJECT_SOURCE_DIR}/example/example1.xyz)
endif()
