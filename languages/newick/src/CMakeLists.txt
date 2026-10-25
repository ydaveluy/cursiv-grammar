# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `newickGenerated` (the compiled library) and `newick_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID newick
    GRAMMAR    newick/core/NewickGrammar.hpp
    NAMESPACE  newick::Newick
    EXTENSIONS .nwk
    OUT        newick/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/newick/core/*.cpp)
add_library(newickCore ${_core})
target_link_libraries(newickCore PUBLIC newickGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/newick/cli/*.cpp)
add_executable(newick_cli ${_cli})
target_link_libraries(newick_cli
                      PRIVATE newickCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/newick/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/newick/lsp/main.cpp)
add_library(newickLsp ${_lsp})
target_link_libraries(newickLsp PUBLIC newickCore cursiv::lsp)
add_executable(newick_lsp newick/lsp/main.cpp)
target_link_libraries(newick_lsp PRIVATE newickLsp)

cursiv_add_benchmark(newick SOURCES newick/bench/main.cpp)

cursiv_package_language(newick)
