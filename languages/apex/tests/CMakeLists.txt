# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every upstream example parses clean, my own documents
# (exercising constructs the corpus never reaches) parse clean too, and real
# field values read back correctly.
add_executable(apex_grammar_test grammar_test.cpp)
target_link_libraries(apex_grammar_test PRIVATE apexCore)
target_compile_definitions(apex_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME apex_grammar_test COMMAND apex_grammar_test)

# ...and the command line reads one upstream example.
if(TARGET apex_cli)
  add_test(NAME apex_cli_test COMMAND apex_cli
           ${PROJECT_SOURCE_DIR}/example/ExampleRunAs.cls)
endif()
