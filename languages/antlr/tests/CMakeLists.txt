# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses as expected.
add_executable(antlr_grammar_test grammar_test.cpp)
target_link_libraries(antlr_grammar_test PRIVATE antlrCore)
target_compile_definitions(antlr_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME antlr_grammar_test COMMAND antlr_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET antlr_cli)
  add_test(NAME antlr_cli_test COMMAND antlr_cli
           ${PROJECT_SOURCE_DIR}/example/ANTLRv4Parser.g4)
endif()
