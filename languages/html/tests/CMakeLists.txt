# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean and builds a Document.
add_executable(html_grammar_test grammar_test.cpp)
target_link_libraries(html_grammar_test PRIVATE htmlCore cursiv::test cursiv::lsp)
target_compile_definitions(html_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME html_grammar_test COMMAND html_grammar_test)

add_executable(html_native_regressions native_regressions.cpp)
target_link_libraries(html_native_regressions PRIVATE htmlCore cursiv::test)
add_test(NAME html_native_regressions COMMAND html_native_regressions)

# ...and the command line reads one of the same examples.
if(TARGET html_cli)
  add_test(NAME html_cli_test COMMAND html_cli
           ${PROJECT_SOURCE_DIR}/example/html4.html)
endif()
