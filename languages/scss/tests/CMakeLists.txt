# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean, and a few
# real field values read back right.
add_executable(scss_grammar_test grammar_test.cpp)
target_link_libraries(scss_grammar_test PRIVATE scssCore)
target_compile_definitions(scss_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME scss_grammar_test COMMAND scss_grammar_test)

# ...and the command line reads one clean sample.
if(TARGET scss_cli)
  add_test(NAME scss_cli_test COMMAND scss_cli
           ${PROJECT_SOURCE_DIR}/example/basic.scss)
endif()
