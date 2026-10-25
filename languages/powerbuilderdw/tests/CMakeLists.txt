# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

add_executable(powerbuilderdw_grammar_test grammar_test.cpp)
target_link_libraries(powerbuilderdw_grammar_test PRIVATE powerbuilderdwCore)
target_compile_definitions(powerbuilderdw_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME powerbuilderdw_grammar_test COMMAND powerbuilderdw_grammar_test)

# ...and the command line reads one sample too.
if(TARGET powerbuilderdw_cli)
  add_test(NAME powerbuilderdw_cli_test COMMAND powerbuilderdw_cli
           ${PROJECT_SOURCE_DIR}/example/d_example_detail_events.srd)
endif()
