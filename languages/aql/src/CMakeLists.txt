# Generate the parser and its language module.
cursiv_generate_language(ID aql
    GRAMMAR    aql/core/AqlGrammar.hpp
    NAMESPACE  aql::Aql
    EXTENSIONS .aql
    OUT        aql/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aql/core/*.cpp)
add_library(aqlCore ${_core})
target_link_libraries(aqlCore PUBLIC aqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aql/cli/*.cpp)
if(_cli)
  add_executable(aql_cli ${_cli})
  target_link_libraries(aql_cli
                        PRIVATE aqlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/aql/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/aql/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/aql/lsp/main.cpp)
  add_library(aqlLsp ${_lsp})
  target_link_libraries(aqlLsp PUBLIC aqlCore cursiv::lsp)
  add_executable(aql_lsp aql/lsp/main.cpp)
  target_link_libraries(aql_lsp PRIVATE aqlLsp)
endif()

if(TARGET aqlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/aql/bench/main.cpp")
  cursiv_add_benchmark(aql
      SOURCES aql/bench/main.cpp)
endif()

cursiv_package_language(aql)
