# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and each builds a Document.
add_executable(xml_grammar_test grammar_test.cpp)
target_link_libraries(xml_grammar_test PRIVATE xmlCore)
target_compile_definitions(xml_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME xml_grammar_test COMMAND xml_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET xml_cli)
  add_test(NAME xml_cli_test COMMAND xml_cli
           ${PROJECT_SOURCE_DIR}/example/books.xml)
endif()
