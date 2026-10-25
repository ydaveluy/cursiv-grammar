# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and each tree roots at File.
add_executable(swift_grammar_test grammar_test.cpp)
target_link_libraries(swift_grammar_test PRIVATE swiftCore cursiv::test)
target_compile_definitions(swift_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME swift_grammar_test COMMAND swift_grammar_test)

# ...and the command line reads one example.
if(TARGET swift_cli)
  add_test(NAME swift_cli_test COMMAND swift_cli
           ${PROJECT_SOURCE_DIR}/example/SimpleValues.swift)
endif()
