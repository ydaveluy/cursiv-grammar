# Generate the parser and its language module.
cursiv_generate_language(ID redis
    GRAMMAR    redis/core/RedisGrammar.hpp
    NAMESPACE  redis::Redis
    EXTENSIONS .redis
    OUT        redis/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/redis/core/*.cpp)
add_library(redisCore ${_core})
target_link_libraries(redisCore PUBLIC redisGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/redis/cli/*.cpp)
if(_cli)
  add_executable(redis_cli ${_cli})
  target_link_libraries(redis_cli
                        PRIVATE redisCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/redis/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/redis/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/redis/lsp/main.cpp)
  add_library(redisLsp ${_lsp})
  target_link_libraries(redisLsp PUBLIC redisCore cursiv::lsp)
  add_executable(redis_lsp redis/lsp/main.cpp)
  target_link_libraries(redis_lsp PRIVATE redisLsp)
endif()

if(TARGET redisLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/redis/bench/main.cpp")
  cursiv_add_benchmark(redis
      SOURCES redis/bench/main.cpp)
endif()

cursiv_package_language(redis)
