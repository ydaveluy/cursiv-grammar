file(GLOB _samples CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/example/*.kt)
list(JOIN _samples "," _samples_joined)

# label-declaration.kt, member-access-rhs.kt and supertype-lambda.kt live
# beside this test, not in the corpus: they cover constructs no upstream
# sample exercises.
file(GLOB _test_samples CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.kt)
list(JOIN _test_samples "," _test_samples_joined)

# The grammar itself: every sample parses, and the tree holds a KotlinFile.
add_executable(kotlin_grammar_test grammar_test.cpp)
target_link_libraries(kotlin_grammar_test PRIVATE kotlinCore)
target_compile_definitions(kotlin_grammar_test PRIVATE
    "CURSIV_SAMPLES=\"${_samples_joined}\""
    "CURSIV_TEST_SAMPLES=\"${_test_samples_joined}\"")
add_test(NAME kotlin_grammar_test COMMAND kotlin_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME kotlin_cli_test COMMAND kotlin_cli
         ${PROJECT_SOURCE_DIR}/example/fix-3208.kt)
