# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

add_executable(tinymud_grammar_test grammar_test.cpp)
target_link_libraries(tinymud_grammar_test PRIVATE tinymudCore)
target_compile_definitions(tinymud_grammar_test PRIVATE
    CURSIV_EXAMPLE1="${PROJECT_SOURCE_DIR}/example/example1.tinymud"
    CURSIV_ADMIN="${CMAKE_CURRENT_SOURCE_DIR}/admin.tinymud"
    CURSIV_ACTIONS="${CMAKE_CURRENT_SOURCE_DIR}/actions.tinymud"
    CURSIV_NEGATIVE_BLANK_LINE="${PROJECT_SOURCE_DIR}/tests/negative/blank-line.tinymud")
add_test(NAME tinymud_grammar_test COMMAND tinymud_grammar_test)

# ...and the command line reads one example too.
if(TARGET tinymud_cli)
  add_test(NAME tinymud_cli_test COMMAND tinymud_cli
           ${PROJECT_SOURCE_DIR}/example/example1.tinymud)
endif()
