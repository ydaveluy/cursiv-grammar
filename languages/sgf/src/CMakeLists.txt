# Generate the parser and its language module.
cursiv_generate_language(ID sgf
    GRAMMAR    sgf/core/SgfGrammar.hpp
    NAMESPACE  sgf::Sgf
    EXTENSIONS .sgf
    OUT        sgf/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sgf/core/*.cpp)
add_library(sgfCore ${_core})
target_link_libraries(sgfCore PUBLIC sgfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sgf/cli/*.cpp)
if(_cli)
  add_executable(sgf_cli ${_cli})
  target_link_libraries(sgf_cli
                        PRIVATE sgfCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sgf/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/sgf/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sgf/lsp/main.cpp)
  add_library(sgfLsp ${_lsp})
  target_link_libraries(sgfLsp PUBLIC sgfCore cursiv::lsp)
  add_executable(sgf_lsp sgf/lsp/main.cpp)
  target_link_libraries(sgf_lsp PRIVATE sgfLsp)
endif()

if(TARGET sgfLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sgf/bench/main.cpp")
  cursiv_add_benchmark(sgf
      SOURCES sgf/bench/main.cpp)
endif()

cursiv_package_language(sgf)
