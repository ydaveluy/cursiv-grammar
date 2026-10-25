# Generate the parser and its language module.
cursiv_generate_language(ID cypher
    GRAMMAR    cypher/core/CypherGrammar.hpp
    NAMESPACE  cypher::Cypher
    EXTENSIONS .cypher
    OUT        cypher/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cypher/core/*.cpp)
add_library(cypherCore ${_core})
target_link_libraries(cypherCore PUBLIC cypherGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cypher/cli/*.cpp)
if(_cli)
  add_executable(cypher_cli ${_cli})
  target_link_libraries(cypher_cli
                        PRIVATE cypherCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cypher/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cypher/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cypher/lsp/main.cpp)
  add_library(cypherLsp ${_lsp})
  target_link_libraries(cypherLsp PUBLIC cypherCore cursiv::lsp)
  add_executable(cypher_lsp cypher/lsp/main.cpp)
  target_link_libraries(cypher_lsp PRIVATE cypherLsp)
endif()

if(TARGET cypherLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cypher/bench/main.cpp")
  cursiv_add_benchmark(cypher
      SOURCES cypher/bench/main.cpp)
endif()

cursiv_package_language(cypher)
