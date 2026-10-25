# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus and coverage document parses, and field
# values come back through the converters that built them.
add_executable(wkt_grammar_test grammar_test.cpp)
target_link_libraries(wkt_grammar_test PRIVATE wktCore cursiv::test cursiv::lsp)
target_compile_definitions(wkt_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME wkt_grammar_test COMMAND wkt_grammar_test)

# ...and the command line reads one document from the same corpus.
if(TARGET wkt_cli)
  add_test(NAME wkt_cli_test COMMAND wkt_cli
           ${PROJECT_SOURCE_DIR}/example/example1.wkt)
endif()

add_executable(wkt_local_dimensions_test local_dimensions_test.cpp)
target_link_libraries(wkt_local_dimensions_test PRIVATE wktCore cursiv::test)
add_test(NAME wkt_local_dimensions_test COMMAND wkt_local_dimensions_test)
