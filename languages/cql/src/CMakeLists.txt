# Generate the parser and its language module.
cursiv_generate_language(ID cql
    GRAMMAR    cql/core/CqlGrammar.hpp
    NAMESPACE  cql::Cql
    EXTENSIONS .cql
    OUT        cql/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cql/core/*.cpp)
add_library(cqlCore ${_core})
target_link_libraries(cqlCore PUBLIC cqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cql/cli/*.cpp)
if(_cli)
  add_executable(cql_cli ${_cli})
  target_link_libraries(cql_cli
                        PRIVATE cqlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cql/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cql/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cql/lsp/main.cpp)
  add_library(cqlLsp ${_lsp})
  target_link_libraries(cqlLsp PUBLIC cqlCore cursiv::lsp)
  add_executable(cql_lsp cql/lsp/main.cpp)
  target_link_libraries(cql_lsp PRIVATE cqlLsp)
endif()

if(TARGET cqlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cql/bench/main.cpp")
  cursiv_add_benchmark(cql
      SOURCES cql/bench/main.cpp)
endif()

cursiv_package_language(cql)
