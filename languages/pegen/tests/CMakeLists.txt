# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds what it declares.
add_executable(pegen_grammar_test grammar_test.cpp)
target_link_libraries(pegen_grammar_test PRIVATE pegenCore cursiv::test)
target_compile_definitions(pegen_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME pegen_grammar_test COMMAND pegen_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET pegen_cli)
  add_test(NAME pegen_cli_test COMMAND pegen_cli
           ${PROJECT_SOURCE_DIR}/example/python-10.gram)
endif()
