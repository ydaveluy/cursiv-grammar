# The grammar itself: every example parses clean, rooted at Doc.
add_executable(json_grammar_test grammar_test.cpp)
target_link_libraries(json_grammar_test PRIVATE jsonCore cursiv::test cursiv::lsp)
target_compile_definitions(json_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME json_grammar_test COMMAND json_grammar_test)

# ...and the command line reads one of them.
add_test(NAME json_cli_test COMMAND json_cli
         ${PROJECT_SOURCE_DIR}/example/example1.json)

# The table engine's own lanes: they read its types or pin its repairs, so they
# exist only where the language is generated for it.
get_target_property(_jsonEngine jsonGenerated CURSIV_ENGINE)
if(_jsonEngine STREQUAL "LR")
  # A string refused in its middle ends at its own closer.
  add_executable(json_refused_escape_test refused_escape_test.cpp)
  target_link_libraries(json_refused_escape_test PRIVATE jsonCore)
  add_test(NAME json_refused_escape_test COMMAND json_refused_escape_test)

  # The repair search on a stray opener and on two closers of different kinds.
  add_executable(json_repair_search_test repair_search_test.cpp)
  target_link_libraries(json_repair_search_test PRIVATE jsonCore)
  add_test(NAME json_repair_search_test COMMAND json_repair_search_test)
endif()
