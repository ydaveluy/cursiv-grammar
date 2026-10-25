# The grammar itself: every example parses, and the tree holds a
# TranslationUnit.
add_executable(c_grammar_test grammar_test.cpp)
target_link_libraries(c_grammar_test PRIVATE cCore)
target_compile_definitions(c_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME c_grammar_test COMMAND c_grammar_test)

# ...and the command line reads one of them.
add_test(NAME c_cli_test COMMAND c_cli
         ${PROJECT_SOURCE_DIR}/example/add.c)

# A reserved word read as a name is the validator's to refuse.
add_executable(c_reserved_test reserved_test.cpp)
target_link_libraries(c_reserved_test PRIVATE cCore)
add_test(NAME c_reserved_test COMMAND c_reserved_test)

# The table engine's own lanes: they read its types, so they exist only where
# the language is generated for it.
get_target_property(_cEngine cGenerated CURSIV_ENGINE)
if(_cEngine STREQUAL "LR")
  # `commit` on the dangling `else`: the nearest `if`, and no branch
  # however deep the nest.
  add_executable(c_commit_test commit_test.cpp)
  target_link_libraries(c_commit_test PRIVATE cCore)
  add_test(NAME c_commit_test COMMAND c_commit_test)

  # A repair budget cut to any floor, and one spent on a comment left open.
  add_executable(c_repair_budget_test repair_budget_test.cpp)
  target_link_libraries(c_repair_budget_test PRIVATE cCore)
  add_test(NAME c_repair_budget_test COMMAND c_repair_budget_test)

  # The interpreted writer against the drive, over every example.
  add_executable(c_writes_through_test writes_through_test.cpp)
  target_link_libraries(c_writes_through_test PRIVATE cCore)
  target_compile_definitions(c_writes_through_test PRIVATE
      CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
  add_test(NAME c_writes_through_test COMMAND c_writes_through_test)
endif()
