# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses, and its tree
# holds a command.
add_executable(sieve_grammar_test grammar_test.cpp)
target_link_libraries(sieve_grammar_test PRIVATE sieveCore)
target_compile_definitions(sieve_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME sieve_grammar_test COMMAND sieve_grammar_test)

# ...and the command line reads one of them.
if(TARGET sieve_cli)
  add_test(NAME sieve_cli_test COMMAND sieve_cli
           ${PROJECT_SOURCE_DIR}/example/example1.sieve)
endif()
