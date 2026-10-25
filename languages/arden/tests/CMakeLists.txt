# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds what it declares.
add_executable(arden_grammar_test grammar_test.cpp)
target_link_libraries(arden_grammar_test PRIVATE ardenCore)
target_compile_definitions(arden_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME arden_grammar_test COMMAND arden_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET arden_cli)
  add_test(NAME arden_cli_test COMMAND arden_cli
           "${PROJECT_SOURCE_DIR}/example/Contraindication Alert.mlm")
endif()
