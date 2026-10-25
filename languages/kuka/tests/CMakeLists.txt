# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every upstream example parses clean, my own documents
# (exercising constructs the one-file corpus never reaches) parse clean too,
# and real field values read back correctly.
add_executable(kuka_grammar_test grammar_test.cpp)
target_link_libraries(kuka_grammar_test PRIVATE kukaCore)
target_compile_definitions(kuka_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME kuka_grammar_test COMMAND kuka_grammar_test)

# ...and the command line reads the upstream example.
if(TARGET kuka_cli)
  add_test(NAME kuka_cli_test COMMAND kuka_cli
           ${PROJECT_SOURCE_DIR}/example/example1.kuka)
endif()
