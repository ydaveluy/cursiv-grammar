# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(ron_grammar_test grammar_test.cpp)
target_link_libraries(ron_grammar_test PRIVATE ronCore)
target_compile_definitions(ron_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ron_grammar_test COMMAND ron_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET ron_cli)
  add_test(NAME ron_cli_test COMMAND ron_cli
           ${PROJECT_SOURCE_DIR}/example/example1.ron)
endif()
