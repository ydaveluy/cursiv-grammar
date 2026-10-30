# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds a File.
add_executable(redcode_grammar_test grammar_test.cpp)
target_link_libraries(redcode_grammar_test PRIVATE redcodeCore cursiv::test)
target_compile_definitions(redcode_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME redcode_grammar_test COMMAND redcode_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET redcode_cli)
  add_test(NAME redcode_cli_test COMMAND redcode_cli
           ${PROJECT_SOURCE_DIR}/example/imp.redcode)
endif()
