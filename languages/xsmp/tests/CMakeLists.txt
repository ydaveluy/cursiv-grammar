# The lanes of this language: one binary per lane, the project directory
# is where a corpus is read from.

# The grammar itself: what this language takes, and what it refuses.
add_executable(xsmp_grammar_test grammar_test.cpp)
target_link_libraries(xsmp_grammar_test PRIVATE xsmpCore)
target_compile_definitions(xsmp_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME xsmp_grammar_test COMMAND xsmp_grammar_test)

# ...and the command line reads the demo catalogue.
add_test(NAME xsmp_cli_test COMMAND xsmp_cli
         ${PROJECT_SOURCE_DIR}/example/demo.xsmpcat)

# The emitted grammar text, read back through the language's own source.
add_executable(xsmp_grammar_source_test grammar_source_test.cpp)
target_link_libraries(xsmp_grammar_source_test PRIVATE xsmpCore)
get_target_property(_xsmpGenDir xsmpGenerated CURSIV_GENERATED_DIR)
target_compile_definitions(xsmp_grammar_source_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_GENERATED_DIR="${_xsmpGenDir}")
add_test(NAME xsmp_grammar_source_test COMMAND xsmp_grammar_source_test)

# The structural fuzz lane: documents written from the declarative grammar and
# the tree each one owes, plus what the table answers where the engine has one.
add_executable(xsmp_fuzz_test fuzz_test.cpp)
target_link_libraries(xsmp_fuzz_test PRIVATE xsmpCore)
target_compile_definitions(xsmp_fuzz_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME xsmp_fuzz_test COMMAND xsmp_fuzz_test)

# The catalogues, linked: what a name resolves to when the document is built
# rather than merely parsed.
add_executable(xsmp_linking_test linking_test.cpp)
target_link_libraries(xsmp_linking_test PRIVATE xsmpCore cursiv::test)
target_compile_definitions(xsmp_linking_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME xsmp_linking_test COMMAND xsmp_linking_test)

# The recovery corpus: a broken document per family, the valid twin they were
# cut from, and the pins the repair is held to.
add_executable(xsmp_recovery_test recovery_test.cpp)
target_link_libraries(xsmp_recovery_test PRIVATE xsmpCore)
target_compile_definitions(xsmp_recovery_test PRIVATE
    CURSIV_RECOVERY_CORPUS="${CMAKE_CURRENT_SOURCE_DIR}/recovery")
add_test(NAME xsmp_recovery_test COMMAND xsmp_recovery_test)

# The table engine's own lanes: they read its types or pin its answers, so they
# exist only where the language is generated for it.
get_target_property(_xsmpEngine xsmpGenerated CURSIV_ENGINE)
if(_xsmpEngine STREQUAL "LR")
  # The caret glued to a keyword that names a parameter.
  add_executable(xsmp_caret_glued_test caret_glued_test.cpp)
  target_link_libraries(xsmp_caret_glued_test PRIVATE xsmpCore)
  add_test(NAME xsmp_caret_glued_test COMMAND xsmp_caret_glued_test)

  # A repair budget cut to any floor: the parse still ends accepted.
  add_executable(xsmp_repair_budget_test repair_budget_test.cpp)
  target_link_libraries(xsmp_repair_budget_test PRIVATE xsmpCore)
  add_test(NAME xsmp_repair_budget_test COMMAND xsmp_repair_budget_test)

  # The interpreted writer against the drive, over whole catalogues.
  add_executable(xsmp_writes_through_test writes_through_test.cpp)
  target_link_libraries(xsmp_writes_through_test PRIVATE xsmpCore)
  target_compile_definitions(xsmp_writes_through_test PRIVATE
      CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}")
  add_test(NAME xsmp_writes_through_test COMMAND xsmp_writes_through_test)
endif()
