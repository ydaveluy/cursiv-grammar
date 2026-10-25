# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every sample parses, and each tree holds a DoiUri root.
add_executable(doiurl_grammar_test grammar_test.cpp)
target_link_libraries(doiurl_grammar_test PRIVATE doiurlCore)
target_compile_definitions(doiurl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME doiurl_grammar_test COMMAND doiurl_grammar_test)

# ...and the command line reads one of them.
if(TARGET doiurl_cli)
  add_test(NAME doiurl_cli_test COMMAND doiurl_cli
           ${PROJECT_SOURCE_DIR}/example/example1.doiurl)
endif()
