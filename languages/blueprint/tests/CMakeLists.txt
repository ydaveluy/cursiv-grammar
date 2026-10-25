# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean.
add_executable(blueprint_grammar_test grammar_test.cpp)
target_link_libraries(blueprint_grammar_test PRIVATE blueprintCore)
add_test(NAME blueprint_grammar_test COMMAND blueprint_grammar_test
         "${PROJECT_SOURCE_DIR}/example/example1.bp"
         "${CMAKE_CURRENT_SOURCE_DIR}/features.bp")

# ...and the command line reads one of the same examples.
if(TARGET blueprint_cli)
  add_test(NAME blueprint_cli_test COMMAND blueprint_cli
           ${PROJECT_SOURCE_DIR}/example/example1.bp)
endif()

add_executable(blueprint_native_values_test native_values_test.cpp)
target_link_libraries(blueprint_native_values_test PRIVATE blueprintCore cursiv::test)
add_test(NAME blueprint_native_values_test COMMAND blueprint_native_values_test)
