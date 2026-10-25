# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document of example/ parses clean.
add_executable(alpaca_grammar_test grammar_test.cpp)
target_link_libraries(alpaca_grammar_test PRIVATE alpacaCore)
target_compile_definitions(alpaca_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME alpaca_grammar_test COMMAND alpaca_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET alpaca_cli)
  add_test(NAME alpaca_cli_test COMMAND alpaca_cli
           ${PROJECT_SOURCE_DIR}/example/conway.alp)
endif()
