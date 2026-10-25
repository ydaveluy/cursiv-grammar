# Generate the parser and its language module.
cursiv_generate_language(ID gql
    GRAMMAR    gql/core/GqlGrammar.hpp
    NAMESPACE  gql::Gql
    EXTENSIONS .gql
    OUT        gql/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gql/core/*.cpp)
add_library(gqlCore ${_core})
target_link_libraries(gqlCore PUBLIC gqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gql/cli/*.cpp)
if(_cli)
  add_executable(gql_cli ${_cli})
  target_link_libraries(gql_cli
                        PRIVATE gqlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/gql/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/gql/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gql/lsp/main.cpp)
  add_library(gqlLsp ${_lsp})
  target_link_libraries(gqlLsp PUBLIC gqlCore cursiv::lsp)
  add_executable(gql_lsp gql/lsp/main.cpp)
  target_link_libraries(gql_lsp PRIVATE gqlLsp)
endif()

if(TARGET gqlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gql/bench/main.cpp")
  cursiv_add_benchmark(gql
      SOURCES gql/bench/main.cpp)
endif()

cursiv_package_language(gql)
