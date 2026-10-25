file(GLOB _samples CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/example/*.py)
list(JOIN _samples "," _samples_joined)

# match_case.py, new_syntax.py and unicode_names.py live beside this test,
# not in the corpus: they cover constructs no upstream sample exercises.
file(GLOB _test_samples CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.py)
list(JOIN _test_samples "," _test_samples_joined)

# The grammar itself: every sample parses, and the tree holds a Module.
add_executable(python_grammar_test grammar_test.cpp)
target_link_libraries(python_grammar_test PRIVATE pythonCore)
target_compile_definitions(python_grammar_test PRIVATE
    "CURSIV_SAMPLES=\"${_samples_joined}\""
    "CURSIV_TEST_SAMPLES=\"${_test_samples_joined}\"")
add_test(NAME python_grammar_test COMMAND python_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME python_cli_test COMMAND python_cli
         ${PROJECT_SOURCE_DIR}/example/__hello__.py)
