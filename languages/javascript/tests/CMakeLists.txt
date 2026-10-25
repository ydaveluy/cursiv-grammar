# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds a Program.
add_executable(javascript_grammar_test grammar_test.cpp)
target_link_libraries(javascript_grammar_test PRIVATE javascriptCore)
target_compile_definitions(javascript_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME javascript_grammar_test COMMAND javascript_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET javascript_cli)
  add_test(NAME javascript_cli_test COMMAND javascript_cli
           ${PROJECT_SOURCE_DIR}/example/Classes.js)
endif()

# What the grammar accepts and the language refuses: strict mode.
add_executable(javascript_strict_test strict_test.cpp)
target_link_libraries(javascript_strict_test PRIVATE javascriptCore)
add_test(NAME javascript_strict_test COMMAND javascript_strict_test)
