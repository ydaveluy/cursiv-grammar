# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `bibtexGenerated` (the compiled library) and `bibtex_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID bibtex
    GRAMMAR    bibtex/core/BibtexGrammar.hpp
    NAMESPACE  bibtex::Bibtex
    EXTENSIONS .bib
    OUT        bibtex/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bibtex/core/*.cpp)
add_library(bibtexCore ${_core})
target_link_libraries(bibtexCore PUBLIC bibtexGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bibtex/cli/*.cpp)
add_executable(bibtex_cli ${_cli})
target_link_libraries(bibtex_cli
                      PRIVATE bibtexCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bibtex/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bibtex/lsp/main.cpp)
add_library(bibtexLsp ${_lsp})
target_link_libraries(bibtexLsp PUBLIC bibtexCore cursiv::lsp)
add_executable(bibtex_lsp bibtex/lsp/main.cpp)
target_link_libraries(bibtex_lsp PRIVATE bibtexLsp)

cursiv_add_benchmark(bibtex SOURCES bibtex/bench/main.cpp)

cursiv_package_language(bibtex)
