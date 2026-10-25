# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(postalcode_grammar_test grammar_test.cpp)
target_link_libraries(postalcode_grammar_test PRIVATE postalcodeCore)
target_compile_definitions(postalcode_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME postalcode_grammar_test COMMAND postalcode_grammar_test)

# ...and the command line reads one of them.
if(TARGET postalcode_cli)
  add_test(NAME postalcode_cli_test COMMAND postalcode_cli
           ${PROJECT_SOURCE_DIR}/example/example1.postalcode)
endif()
