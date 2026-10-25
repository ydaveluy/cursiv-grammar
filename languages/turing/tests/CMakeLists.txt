# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(turing_grammar_test grammar_test.cpp)
target_link_libraries(turing_grammar_test PRIVATE turingCore)
target_compile_definitions(turing_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME turing_grammar_test COMMAND turing_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET turing_cli)
  add_test(NAME turing_cli_test COMMAND turing_cli
           ${PROJECT_SOURCE_DIR}/example/example1.t)
endif()
