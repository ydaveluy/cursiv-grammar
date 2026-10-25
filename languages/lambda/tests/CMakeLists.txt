# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into an Expression.
add_executable(lambda_grammar_test grammar_test.cpp)
target_link_libraries(lambda_grammar_test PRIVATE lambdaCore)
target_compile_definitions(lambda_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME lambda_grammar_test COMMAND lambda_grammar_test)

# ...and the command line reads one example.
if(TARGET lambda_cli)
  add_test(NAME lambda_cli_test COMMAND lambda_cli
           ${PROJECT_SOURCE_DIR}/example/example1.lambda)
endif()
