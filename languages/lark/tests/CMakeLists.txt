# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(lark_grammar_test grammar_test.cpp)
target_link_libraries(lark_grammar_test PRIVATE larkCore cursiv::test cursiv::lsp)
target_compile_definitions(lark_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME lark_grammar_test COMMAND lark_grammar_test)

# ...and the command line reads one of them.
if(TARGET lark_cli)
  add_test(NAME lark_cli_test COMMAND lark_cli
           ${PROJECT_SOURCE_DIR}/example/json.lark)
endif()
