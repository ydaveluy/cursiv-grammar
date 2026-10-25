# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document in example/ parses, and the tree holds
# what it declares.
add_executable(gdscript_grammar_test grammar_test.cpp)
target_link_libraries(gdscript_grammar_test PRIVATE gdscriptCore)
target_compile_definitions(gdscript_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME gdscript_grammar_test COMMAND gdscript_grammar_test)

# ...and the command line reads one sample from the same directory.
if(TARGET gdscript_cli)
  add_test(NAME gdscript_cli_test COMMAND gdscript_cli
           ${PROJECT_SOURCE_DIR}/example/getting_started.gd)
endif()
