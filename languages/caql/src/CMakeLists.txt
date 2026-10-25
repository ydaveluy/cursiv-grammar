# Generate the parser and its language module.
cursiv_generate_language(ID caql
    GRAMMAR    caql/core/CaqlGrammar.hpp
    NAMESPACE  caql::Caql
    EXTENSIONS .caql
    OUT        caql/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/caql/core/*.cpp)
add_library(caqlCore ${_core})
target_link_libraries(caqlCore PUBLIC caqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/caql/cli/*.cpp)
if(_cli)
  add_executable(caql_cli ${_cli})
  target_link_libraries(caql_cli
                        PRIVATE caqlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/caql/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/caql/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/caql/lsp/main.cpp)
  add_library(caqlLsp ${_lsp})
  target_link_libraries(caqlLsp PUBLIC caqlCore cursiv::lsp)
  add_executable(caql_lsp caql/lsp/main.cpp)
  target_link_libraries(caql_lsp PRIVATE caqlLsp)
endif()

if(TARGET caqlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/caql/bench/main.cpp")
  cursiv_add_benchmark(caql
      SOURCES caql/bench/main.cpp)
endif()

cursiv_package_language(caql)
