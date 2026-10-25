# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document parses clean, features.mod
# covers what the corpus never reaches, and hello.mod is read back field by
# field.
add_executable(oberon_grammar_test grammar_test.cpp)
target_link_libraries(oberon_grammar_test PRIVATE oberonCore)
target_compile_definitions(oberon_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_FEATURES_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/features.mod")
add_test(NAME oberon_grammar_test COMMAND oberon_grammar_test)

# ...and the command line reads one document from the same corpus.
if(TARGET oberon_cli)
  add_test(NAME oberon_cli_test COMMAND oberon_cli
           ${PROJECT_SOURCE_DIR}/example/hello.mod)
endif()
