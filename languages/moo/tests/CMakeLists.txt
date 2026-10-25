# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document parses, and hello.moo is read
# back field by field.
add_executable(moo_grammar_test grammar_test.cpp)
target_link_libraries(moo_grammar_test PRIVATE mooCore)
target_compile_definitions(moo_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_FEATURES_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/features.moo")
add_test(NAME moo_grammar_test COMMAND moo_grammar_test)

# ...and the command line reads one document from the same corpus.
if(TARGET moo_cli)
  add_test(NAME moo_cli_test COMMAND moo_cli
           ${PROJECT_SOURCE_DIR}/example/hello.moo)
endif()
