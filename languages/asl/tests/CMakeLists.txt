# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses, and a few real
# field values read back what the source declares.
add_executable(asl_grammar_test grammar_test.cpp)
target_link_libraries(asl_grammar_test PRIVATE aslCore)
target_compile_definitions(asl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME asl_grammar_test COMMAND asl_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET asl_cli)
  add_test(NAME asl_cli_test COMMAND asl_cli
           ${PROJECT_SOURCE_DIR}/example/assignments.asl)
endif()
