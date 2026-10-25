# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `folGenerated` (the compiled library) and `fol_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID fol
    GRAMMAR    fol/core/FolGrammar.hpp
    NAMESPACE  fol::Fol
    EXTENSIONS .fol
    OUT        fol/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fol/core/*.cpp)
add_library(folCore ${_core})
target_link_libraries(folCore PUBLIC folGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fol/cli/*.cpp)
add_executable(fol_cli ${_cli})
target_link_libraries(fol_cli
                      PRIVATE folCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fol/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/fol/lsp/main.cpp)
add_library(folLsp ${_lsp})
target_link_libraries(folLsp PUBLIC folCore cursiv::lsp)
add_executable(fol_lsp fol/lsp/main.cpp)
target_link_libraries(fol_lsp PRIVATE folLsp)

cursiv_add_benchmark(fol SOURCES fol/bench/main.cpp)

cursiv_package_language(fol)
