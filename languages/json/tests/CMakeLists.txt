# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean, rooted at Doc.
add_executable(json_grammar_test grammar_test.cpp)
target_link_libraries(json_grammar_test PRIVATE jsonCore cursiv::test cursiv::lsp)
target_compile_definitions(json_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME json_grammar_test COMMAND json_grammar_test)

# ...and the command line reads one of them.
if(TARGET json_cli)
  add_test(NAME json_cli_test COMMAND json_cli
           ${PROJECT_SOURCE_DIR}/example/example1.json)
endif()

foreach(lane IN ITEMS refused_escape repair_search)
  add_executable(json_${lane}_test ${lane}_test.cpp)
  target_link_libraries(json_${lane}_test PRIVATE jsonCore cursiv::test)
  add_test(NAME json_${lane}_test COMMAND json_${lane}_test)
endforeach()
