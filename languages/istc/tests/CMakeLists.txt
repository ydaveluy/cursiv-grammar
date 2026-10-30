# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file under example/ parses clean.
add_executable(istc_grammar_test grammar_test.cpp)
target_link_libraries(istc_grammar_test PRIVATE istcCore cursiv::test cursiv::lsp)
target_compile_definitions(istc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME istc_grammar_test COMMAND istc_grammar_test)

# ...and the command line reads one of them.
if(TARGET istc_cli)
  add_test(NAME istc_cli_test COMMAND istc_cli
           ${PROJECT_SOURCE_DIR}/example/example1.istc)
endif()
