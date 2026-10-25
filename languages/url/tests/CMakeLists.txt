# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document of example/ parses, and the tree holds
# a Url.
add_executable(url_grammar_test grammar_test.cpp)
target_link_libraries(url_grammar_test PRIVATE urlCore cursiv::test)
target_compile_definitions(url_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME url_grammar_test COMMAND url_grammar_test)

# ...and the command line reads one of them.
if(TARGET url_cli)
  add_test(NAME url_cli_test COMMAND url_cli
           ${PROJECT_SOURCE_DIR}/example/example10.url)
endif()
