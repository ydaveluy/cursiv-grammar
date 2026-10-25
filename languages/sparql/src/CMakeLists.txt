# Generate the parser and its language module.
cursiv_generate_language(ID sparql
    GRAMMAR    sparql/core/SparqlGrammar.hpp
    NAMESPACE  sparql::Sparql
    EXTENSIONS .rq
    OUT        sparql/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sparql/core/*.cpp)
add_library(sparqlCore ${_core})
target_link_libraries(sparqlCore PUBLIC sparqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sparql/cli/*.cpp)
if(_cli)
  add_executable(sparql_cli ${_cli})
  target_link_libraries(sparql_cli
                        PRIVATE sparqlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sparql/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/sparql/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sparql/lsp/main.cpp)
  add_library(sparqlLsp ${_lsp})
  target_link_libraries(sparqlLsp PUBLIC sparqlCore cursiv::lsp)
  add_executable(sparql_lsp sparql/lsp/main.cpp)
  target_link_libraries(sparql_lsp PRIVATE sparqlLsp)
endif()

if(TARGET sparqlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sparql/bench/main.cpp")
  cursiv_add_benchmark(sparql
      SOURCES sparql/bench/main.cpp)
endif()

cursiv_package_language(sparql)
