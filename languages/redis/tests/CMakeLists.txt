# The grammar itself: every command shape, and the whole corpus.
add_executable(redis_grammar_test grammar_test.cpp)
target_link_libraries(redis_grammar_test PRIVATE redisCore cursiv::test)
target_compile_definitions(redis_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME redis_grammar_test COMMAND redis_grammar_test)

# ...and the command line reads one sample of the corpus.
add_test(NAME redis_cli_test COMMAND redis_cli
         ${PROJECT_SOURCE_DIR}/example/get.redis)

add_executable(redis_native_test native_test.cpp)
target_link_libraries(redis_native_test PRIVATE redisCore cursiv::test)
target_compile_definitions(redis_native_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME redis_native_test COMMAND redis_native_test)
