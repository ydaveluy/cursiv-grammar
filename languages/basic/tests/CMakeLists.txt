# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(basic_grammar_test grammar_test.cpp)
target_link_libraries(basic_grammar_test PRIVATE basicCore)
target_compile_definitions(basic_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME basic_grammar_test COMMAND basic_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET basic_cli)
  add_test(NAME basic_cli_test COMMAND basic_cli
           ${PROJECT_SOURCE_DIR}/example/hello.bas)
endif()
