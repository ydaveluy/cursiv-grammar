# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: both dumps parse, and the fields they write hold.
add_executable(muddb_grammar_test grammar_test.cpp)
target_link_libraries(muddb_grammar_test PRIVATE muddbCore cursiv::test)
target_compile_definitions(muddb_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME muddb_grammar_test COMMAND muddb_grammar_test)

# ...and the command line reads one of the same files.
if(TARGET muddb_cli)
  add_test(NAME muddb_cli_test COMMAND muddb_cli
           ${PROJECT_SOURCE_DIR}/example/minimal.muddb)
endif()
