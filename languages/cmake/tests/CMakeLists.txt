# The grammar itself: every document under example/ parses, and the tree
# holds a File at its root.
add_executable(cmake_grammar_test grammar_test.cpp)
target_link_libraries(cmake_grammar_test PRIVATE cmakeCore)
target_compile_definitions(cmake_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME cmake_grammar_test COMMAND cmake_grammar_test)

add_executable(cmake_native_regressions native_regressions.cpp)
target_link_libraries(cmake_native_regressions PRIVATE cmakeCore cursiv::test)
add_test(NAME cmake_native_regressions COMMAND cmake_native_regressions)

# ...and the command line reads one of the same samples.
add_test(NAME cmake_cli_test COMMAND cmake_cli
         ${PROJECT_SOURCE_DIR}/example/CMakeLists.cmake)
