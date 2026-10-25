# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document in example/ parses, and the tree holds
# what each one declares.
add_executable(orwell_grammar_test grammar_test.cpp)
target_link_libraries(orwell_grammar_test PRIVATE orwellCore)
target_compile_definitions(orwell_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME orwell_grammar_test COMMAND orwell_grammar_test)

# ...and the command line reads one of them.
if(TARGET orwell_cli)
  add_test(NAME orwell_cli_test COMMAND orwell_cli
           ${PROJECT_SOURCE_DIR}/example/example1.orwell)
endif()
