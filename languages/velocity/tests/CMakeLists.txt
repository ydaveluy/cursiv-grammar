# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every upstream example parses, and the test reads a
# few field values back out of the tree.
add_executable(velocity_grammar_test grammar_test.cpp)
target_link_libraries(velocity_grammar_test PRIVATE velocityCore)
target_compile_definitions(velocity_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME velocity_grammar_test COMMAND velocity_grammar_test)

# ...and the command line reads one upstream sample.
if(TARGET velocity_cli)
  add_test(NAME velocity_cli_test COMMAND velocity_cli
           ${PROJECT_SOURCE_DIR}/example/hello_world.vm)
endif()
