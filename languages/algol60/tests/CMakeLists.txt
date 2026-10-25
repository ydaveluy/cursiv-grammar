# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document parses, and one tree holds what
# it declares.
add_executable(algol60_grammar_test grammar_test.cpp)
target_link_libraries(algol60_grammar_test PRIVATE algol60Core)
target_compile_definitions(algol60_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME algol60_grammar_test COMMAND algol60_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET algol60_cli)
  add_test(NAME algol60_cli_test COMMAND algol60_cli
           ${PROJECT_SOURCE_DIR}/example/jensen.a60)
endif()
