# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and a handful of shapes are pinned.
add_executable(lua_grammar_test grammar_test.cpp)
target_link_libraries(lua_grammar_test PRIVATE luaCore)
target_compile_definitions(lua_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME lua_grammar_test COMMAND lua_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET lua_cli)
  add_test(NAME lua_cli_test COMMAND lua_cli
           ${PROJECT_SOURCE_DIR}/example/factorial.lua)
endif()
