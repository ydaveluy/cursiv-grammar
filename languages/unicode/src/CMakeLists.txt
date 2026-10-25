# Generate the parser and its language module.
cursiv_generate_language(ID graphemes
    GRAMMAR    graphemes/core/GraphemesGrammar.hpp
    NAMESPACE  graphemes::Graphemes
    EXTENSIONS .graphemes
    OUT        graphemes/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphemes/core/*.cpp)
add_library(graphemesCore ${_core})
target_link_libraries(graphemesCore PUBLIC graphemesGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphemes/cli/*.cpp)
if(_cli)
  add_executable(graphemes_cli ${_cli})
  target_link_libraries(graphemes_cli
                        PRIVATE graphemesCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/graphemes/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/graphemes/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/graphemes/lsp/main.cpp)
  add_library(graphemesLsp ${_lsp})
  target_link_libraries(graphemesLsp PUBLIC graphemesCore cursiv::lsp)
  add_executable(graphemes_lsp graphemes/lsp/main.cpp)
  target_link_libraries(graphemes_lsp PRIVATE graphemesLsp)
endif()

if(TARGET graphemesLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/graphemes/bench/main.cpp")
  cursiv_add_benchmark(graphemes
      SOURCES graphemes/bench/main.cpp)
endif()

cursiv_package_language(graphemes)
