# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds a
# TranslationUnit.
add_executable(cpp_grammar_test grammar_test.cpp)
target_link_libraries(cpp_grammar_test PRIVATE cppCore cursiv::test cursiv::lsp)
target_compile_definitions(cpp_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME cpp_grammar_test COMMAND cpp_grammar_test)

# ...and the command line reads one of them.
if(TARGET cpp_cli)
  add_test(NAME cpp_cli_test COMMAND cpp_cli
           ${PROJECT_SOURCE_DIR}/example/test1.cpp)
endif()
