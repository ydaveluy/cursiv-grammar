# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ (upstream) and tests/
# (constructs the corpus never reaches) parses clean, and the tree holds
# what it declares.
add_executable(bcpl_grammar_test grammar_test.cpp)
target_link_libraries(bcpl_grammar_test PRIVATE bcplCore)
target_compile_definitions(bcpl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME bcpl_grammar_test COMMAND bcpl_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET bcpl_cli)
  add_test(NAME bcpl_cli_test COMMAND bcpl_cli
           ${PROJECT_SOURCE_DIR}/example/fact.bcpl)
endif()
