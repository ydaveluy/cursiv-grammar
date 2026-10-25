# Generate the parser and its language module.
cursiv_generate_language(ID antlr
    GRAMMAR    antlr/core/AntlrGrammar.hpp
    NAMESPACE  antlr::Antlr
    EXTENSIONS .g4
    OUT        antlr/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/antlr/core/*.cpp)
add_library(antlrCore ${_core})
target_link_libraries(antlrCore PUBLIC antlrGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/antlr/cli/*.cpp)
if(_cli)
  add_executable(antlr_cli ${_cli})
  target_link_libraries(antlr_cli
                        PRIVATE antlrCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/antlr/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/antlr/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/antlr/lsp/main.cpp)
  add_library(antlrLsp ${_lsp})
  target_link_libraries(antlrLsp PUBLIC antlrCore cursiv::lsp)
  add_executable(antlr_lsp antlr/lsp/main.cpp)
  target_link_libraries(antlr_lsp PRIVATE antlrLsp)
endif()

if(TARGET antlrLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/antlr/bench/main.cpp")
  cursiv_add_benchmark(antlr
      SOURCES antlr/bench/main.cpp)
endif()

cursiv_package_language(antlr)
