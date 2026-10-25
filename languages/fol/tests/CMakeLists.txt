# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Condition, and
# functions.fol (this port's own, exercising FuncCall/EqualityAtom) reads
# back field by field.
add_executable(fol_grammar_test grammar_test.cpp)
target_link_libraries(fol_grammar_test PRIVATE folCore)
target_compile_definitions(fol_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME fol_grammar_test COMMAND fol_grammar_test)

# ...and the command line reads one example.
if(TARGET fol_cli)
  add_test(NAME fol_cli_test COMMAND fol_cli
           ${PROJECT_SOURCE_DIR}/example/example1.fol)
endif()
