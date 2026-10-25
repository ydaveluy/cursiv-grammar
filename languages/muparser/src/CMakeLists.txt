# Generate the parser and its language module.
cursiv_generate_language(ID muparser
    GRAMMAR    muparser/core/MuparserGrammar.hpp
    NAMESPACE  muparser::Muparser
    EXTENSIONS .muparser
    OUT        muparser/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/muparser/core/*.cpp)
add_library(muparserCore ${_core})
target_link_libraries(muparserCore PUBLIC muparserGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/muparser/cli/*.cpp)
if(_cli)
  add_executable(muparser_cli ${_cli})
  target_link_libraries(muparser_cli
                        PRIVATE muparserCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/muparser/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/muparser/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/muparser/lsp/main.cpp)
  add_library(muparserLsp ${_lsp})
  target_link_libraries(muparserLsp PUBLIC muparserCore cursiv::lsp)
  add_executable(muparser_lsp muparser/lsp/main.cpp)
  target_link_libraries(muparser_lsp PRIVATE muparserLsp)
endif()

if(TARGET muparserLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/muparser/bench/main.cpp")
  cursiv_add_benchmark(muparser
      SOURCES muparser/bench/main.cpp)
endif()

cursiv_package_language(muparser)
