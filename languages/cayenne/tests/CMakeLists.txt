# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean, rooted at an Expr.
add_executable(cayenne_grammar_test grammar_test.cpp)
target_link_libraries(cayenne_grammar_test PRIVATE cayenneCore)
target_compile_definitions(cayenne_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME cayenne_grammar_test COMMAND cayenne_grammar_test)

# ...and the command line reads one of them.
if(TARGET cayenne_cli)
  add_test(NAME cayenne_cli_test COMMAND cayenne_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/unit.cayenne)
endif()
