# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

add_executable(less_grammar_test grammar_test.cpp)
target_link_libraries(less_grammar_test PRIVATE lessCore)
target_compile_definitions(less_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_FEATURES_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/features.less")
add_test(NAME less_grammar_test COMMAND less_grammar_test)

if(TARGET less_cli)
  add_test(NAME less_cli_test COMMAND less_cli
           ${PROJECT_SOURCE_DIR}/example/example1.less)
endif()
