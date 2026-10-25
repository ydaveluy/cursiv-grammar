# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Document.
add_executable(useragent_grammar_test grammar_test.cpp)
target_link_libraries(useragent_grammar_test PRIVATE useragentCore)
target_compile_definitions(useragent_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME useragent_grammar_test COMMAND useragent_grammar_test)

# ...and the command line reads one example.
if(TARGET useragent_cli)
  add_test(NAME useragent_cli_test COMMAND useragent_cli
           ${PROJECT_SOURCE_DIR}/example/example1.useragent)
endif()
