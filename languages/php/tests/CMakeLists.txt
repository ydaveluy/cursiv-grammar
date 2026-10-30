# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds a Document.
add_executable(php_grammar_test grammar_test.cpp)
target_link_libraries(php_grammar_test PRIVATE phpCore cursiv::test)
target_compile_definitions(php_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME php_grammar_test COMMAND php_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET php_cli)
  add_test(NAME php_cli_test COMMAND php_cli
           ${PROJECT_SOURCE_DIR}/example/hello.php)
endif()
