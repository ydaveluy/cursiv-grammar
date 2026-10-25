# Generate the parser and its language module.
cursiv_generate_language(ID bnf
    GRAMMAR    bnf/core/BnfGrammar.hpp
    NAMESPACE  bnf::Bnf
    EXTENSIONS .bnf
    OUT        bnf/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bnf/core/*.cpp)
add_library(bnfCore ${_core})
target_link_libraries(bnfCore PUBLIC bnfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bnf/cli/*.cpp)
if(_cli)
  add_executable(bnf_cli ${_cli})
  target_link_libraries(bnf_cli
                        PRIVATE bnfCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bnf/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/bnf/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bnf/lsp/main.cpp)
  add_library(bnfLsp ${_lsp})
  target_link_libraries(bnfLsp PUBLIC bnfCore cursiv::lsp)
  add_executable(bnf_lsp bnf/lsp/main.cpp)
  target_link_libraries(bnf_lsp PRIVATE bnfLsp)
endif()

if(TARGET bnfLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bnf/bench/main.cpp")
  cursiv_add_benchmark(bnf
      SOURCES bnf/bench/main.cpp)
endif()

cursiv_package_language(bnf)
