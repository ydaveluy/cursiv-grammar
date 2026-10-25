# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document in example/ parses clean.
add_executable(tiny_grammar_test grammar_test.cpp)
target_link_libraries(tiny_grammar_test PRIVATE tinyCore)
target_compile_definitions(tiny_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME tiny_grammar_test COMMAND tiny_grammar_test)

# ...and the command line reads one of them.
if(TARGET tiny_cli)
  add_test(NAME tiny_cli_test COMMAND tiny_cli
           ${PROJECT_SOURCE_DIR}/example/example1.tiny)
endif()
