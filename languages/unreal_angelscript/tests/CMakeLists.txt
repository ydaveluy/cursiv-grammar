# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds what it
# declares.
add_executable(unrealangelscript_grammar_test grammar_test.cpp)
target_link_libraries(unrealangelscript_grammar_test PRIVATE unrealangelscriptCore)
target_compile_definitions(unrealangelscript_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME unrealangelscript_grammar_test COMMAND unrealangelscript_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET unrealangelscript_cli)
  add_test(NAME unrealangelscript_cli_test COMMAND unrealangelscript_cli
           ${PROJECT_SOURCE_DIR}/example/Example_Actor.as)
endif()
