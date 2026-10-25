# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

add_executable(joss_grammar_test grammar_test.cpp)
target_link_libraries(joss_grammar_test PRIVATE jossCore)
target_compile_definitions(joss_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME joss_grammar_test COMMAND joss_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET joss_cli)
  add_test(NAME joss_cli_test COMMAND joss_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/statements.joss)
endif()
