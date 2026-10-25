# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example under example/ parses, and each tree
# holds a Program.
add_executable(trac_grammar_test grammar_test.cpp)
target_link_libraries(trac_grammar_test PRIVATE tracCore)
target_compile_definitions(trac_grammar_test PRIVATE
    CURSIV_EXAMPLES_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME trac_grammar_test COMMAND trac_grammar_test)

# ...and the command line reads one of them.
if(TARGET trac_cli)
  add_test(NAME trac_cli_test COMMAND trac_cli
           ${PROJECT_SOURCE_DIR}/example/example1.trac)
endif()
