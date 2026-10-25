# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file of example/ parses clean.
add_executable(erlang_grammar_test grammar_test.cpp)
target_link_libraries(erlang_grammar_test PRIVATE erlangCore)
target_compile_definitions(erlang_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME erlang_grammar_test COMMAND erlang_grammar_test)

# ...and the command line reads one sample of the corpus.
if(TARGET erlang_cli)
  add_test(NAME erlang_cli_test COMMAND erlang_cli
           ${PROJECT_SOURCE_DIR}/example/fac.erl)
endif()
