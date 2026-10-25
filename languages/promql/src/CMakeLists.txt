# Generate the parser and its language module.
cursiv_generate_language(ID promql
    GRAMMAR    promql/core/PromqlGrammar.hpp
    NAMESPACE  promql::Promql
    EXTENSIONS .promql
    OUT        promql/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/promql/core/*.cpp)
add_library(promqlCore ${_core})
target_link_libraries(promqlCore PUBLIC promqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/promql/cli/*.cpp)
if(_cli)
  add_executable(promql_cli ${_cli})
  target_link_libraries(promql_cli
                        PRIVATE promqlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/promql/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/promql/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/promql/lsp/main.cpp)
  add_library(promqlLsp ${_lsp})
  target_link_libraries(promqlLsp PUBLIC promqlCore cursiv::lsp)
  add_executable(promql_lsp promql/lsp/main.cpp)
  target_link_libraries(promql_lsp PRIVATE promqlLsp)
endif()

if(TARGET promqlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/promql/bench/main.cpp")
  cursiv_add_benchmark(promql
      SOURCES promql/bench/main.cpp)
endif()

cursiv_package_language(promql)
