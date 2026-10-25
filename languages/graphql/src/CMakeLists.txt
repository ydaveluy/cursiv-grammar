# Generate the parser and its language module.
cursiv_generate_language(ID graphql
    GRAMMAR    graphql/core/GraphqlGrammar.hpp
    NAMESPACE  graphql::Graphql
    EXTENSIONS .graphql
    OUT        graphql/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphql/core/*.cpp)
add_library(graphqlCore ${_core})
target_link_libraries(graphqlCore PUBLIC graphqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphql/cli/*.cpp)
if(_cli)
  add_executable(graphql_cli ${_cli})
  target_link_libraries(graphql_cli
                        PRIVATE graphqlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/graphql/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/graphql/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/graphql/lsp/main.cpp)
  add_library(graphqlLsp ${_lsp})
  target_link_libraries(graphqlLsp PUBLIC graphqlCore cursiv::lsp)
  add_executable(graphql_lsp graphql/lsp/main.cpp)
  target_link_libraries(graphql_lsp PRIVATE graphqlLsp)
endif()

if(TARGET graphqlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/graphql/bench/main.cpp")
  cursiv_add_benchmark(graphql
      SOURCES graphql/bench/main.cpp)
endif()

cursiv_package_language(graphql)
