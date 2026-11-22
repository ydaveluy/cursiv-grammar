# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds a
# TranslationUnit.
add_executable(c_grammar_test grammar_test.cpp)
target_link_libraries(c_grammar_test PRIVATE cCore)
target_compile_definitions(c_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME c_grammar_test COMMAND c_grammar_test)

# ...and the command line reads one of them.
if(TARGET c_cli)
  add_test(NAME c_cli_test COMMAND c_cli
           ${PROJECT_SOURCE_DIR}/example/add.c)
endif()

# A reserved word read as a name is the validator's to refuse.
add_executable(c_reserved_test reserved_test.cpp)
target_link_libraries(c_reserved_test PRIVATE cCore)
add_test(NAME c_reserved_test COMMAND c_reserved_test)

foreach(lane IN ITEMS commit recovery cast)
  add_executable(c_${lane}_test ${lane}_test.cpp)
  target_link_libraries(c_${lane}_test PRIVATE cCore cursiv::test)
  add_test(NAME c_${lane}_test COMMAND c_${lane}_test)
endforeach()
