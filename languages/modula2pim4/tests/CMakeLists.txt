# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: the corpus parses, and the feature documents (what the
# one-file corpus never reaches) parse and read back field by field.
add_executable(modula2pim4_grammar_test grammar_test.cpp)
target_link_libraries(modula2pim4_grammar_test PRIVATE modula2pim4Core)
target_compile_definitions(modula2pim4_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME modula2pim4_grammar_test COMMAND modula2pim4_grammar_test)

# ...and the command line reads one document from the same corpus.
if(TARGET modula2pim4_cli)
  add_test(NAME modula2pim4_cli_test COMMAND modula2pim4_cli
           ${PROJECT_SOURCE_DIR}/example/example1.mod)
endif()
