# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(metric_grammar_test grammar_test.cpp)
target_link_libraries(metric_grammar_test PRIVATE metricCore)
target_compile_definitions(metric_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME metric_grammar_test COMMAND metric_grammar_test)

# ...and the command line reads one of them.
if(TARGET metric_cli)
  add_test(NAME metric_cli_test COMMAND metric_cli
           ${PROJECT_SOURCE_DIR}/example/kmgs2.metric)
endif()
