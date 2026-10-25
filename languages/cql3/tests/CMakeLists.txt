# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(cql3_grammar_test grammar_test.cpp)
target_link_libraries(cql3_grammar_test PRIVATE cql3Core)
target_compile_definitions(cql3_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME cql3_grammar_test COMMAND cql3_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET cql3_cli)
  add_test(NAME cql3_cli_test COMMAND cql3_cli
           ${PROJECT_SOURCE_DIR}/example/select.cql3)
endif()
