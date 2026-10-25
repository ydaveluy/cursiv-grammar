# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(eiffel_grammar_test grammar_test.cpp)
target_link_libraries(eiffel_grammar_test PRIVATE eiffelCore)
target_compile_definitions(eiffel_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME eiffel_grammar_test COMMAND eiffel_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET eiffel_cli)
  add_test(NAME eiffel_cli_test COMMAND eiffel_cli
           ${PROJECT_SOURCE_DIR}/example/100_doors/application.e)
endif()
