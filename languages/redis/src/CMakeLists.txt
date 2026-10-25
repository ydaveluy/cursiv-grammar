# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `redisGenerated` (the compiled library) and `redis_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID redis
    GRAMMAR    redis/core/RedisGrammar.hpp
    NAMESPACE  redis::Redis
    EXTENSIONS .redis
    OUT        redis/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/redis/core/*.cpp)
add_library(redisCore ${_core})
target_link_libraries(redisCore PUBLIC redisGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/redis/cli/*.cpp)
add_executable(redis_cli ${_cli})
target_link_libraries(redis_cli
                      PRIVATE redisCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/redis/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/redis/lsp/main.cpp)
add_library(redisLsp ${_lsp})
target_link_libraries(redisLsp PUBLIC redisCore cursiv::lsp)
add_executable(redis_lsp redis/lsp/main.cpp)
target_link_libraries(redis_lsp PRIVATE redisLsp)

cursiv_add_benchmark(redis SOURCES redis/bench/main.cpp)

cursiv_package_language(redis)
