# Generate the parser and its language module.
cursiv_generate_language(ID bibtex
    GRAMMAR    bibtex/core/BibtexGrammar.hpp
    NAMESPACE  bibtex::Bibtex
    EXTENSIONS .bib
    OUT        bibtex/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bibtex/core/*.cpp)
add_library(bibtexCore ${_core})
target_link_libraries(bibtexCore PUBLIC bibtexGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bibtex/cli/*.cpp)
if(_cli)
  add_executable(bibtex_cli ${_cli})
  target_link_libraries(bibtex_cli
                        PRIVATE bibtexCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bibtex/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/bibtex/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bibtex/lsp/main.cpp)
  add_library(bibtexLsp ${_lsp})
  target_link_libraries(bibtexLsp PUBLIC bibtexCore cursiv::lsp)
  add_executable(bibtex_lsp bibtex/lsp/main.cpp)
  target_link_libraries(bibtex_lsp PRIVATE bibtexLsp)
endif()

if(TARGET bibtexLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bibtex/bench/main.cpp")
  cursiv_add_benchmark(bibtex
      SOURCES bibtex/bench/main.cpp)
endif()

cursiv_package_language(bibtex)
