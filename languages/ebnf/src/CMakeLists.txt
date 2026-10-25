# Generate the parser and its language module.
cursiv_generate_language(ID ebnf
    GRAMMAR    ebnf/core/EbnfGrammar.hpp
    NAMESPACE  ebnf::Ebnf
    EXTENSIONS .ebnf
    OUT        ebnf/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ebnf/core/*.cpp)
add_library(ebnfCore ${_core})
target_link_libraries(ebnfCore PUBLIC ebnfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ebnf/cli/*.cpp)
if(_cli)
  add_executable(ebnf_cli ${_cli})
  target_link_libraries(ebnf_cli
                        PRIVATE ebnfCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ebnf/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/ebnf/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ebnf/lsp/main.cpp)
  add_library(ebnfLsp ${_lsp})
  target_link_libraries(ebnfLsp PUBLIC ebnfCore cursiv::lsp)
  add_executable(ebnf_lsp ebnf/lsp/main.cpp)
  target_link_libraries(ebnf_lsp PRIVATE ebnfLsp)
endif()

if(TARGET ebnfLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ebnf/bench/main.cpp")
  cursiv_add_benchmark(ebnf
      SOURCES ebnf/bench/main.cpp)
endif()

cursiv_package_language(ebnf)
