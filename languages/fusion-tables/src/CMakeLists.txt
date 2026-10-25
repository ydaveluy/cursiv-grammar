# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `fusiontablesGenerated` (the compiled library) and `fusiontables_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID fusiontables
    GRAMMAR    fusiontables/core/FusionTablesGrammar.hpp
    NAMESPACE  fusiontables::FusionTables
    EXTENSIONS .fql
    OUT        fusiontables/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fusiontables/core/*.cpp)
add_library(fusiontablesCore ${_core})
target_link_libraries(fusiontablesCore PUBLIC fusiontablesGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fusiontables/cli/*.cpp)
add_executable(fusiontables_cli ${_cli})
target_link_libraries(fusiontables_cli
                      PRIVATE fusiontablesCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fusiontables/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/fusiontables/lsp/main.cpp)
add_library(fusiontablesLsp ${_lsp})
target_link_libraries(fusiontablesLsp PUBLIC fusiontablesCore cursiv::lsp)
add_executable(fusiontables_lsp fusiontables/lsp/main.cpp)
target_link_libraries(fusiontables_lsp PRIVATE fusiontablesLsp)

cursiv_add_benchmark(fusiontables SOURCES fusiontables/bench/main.cpp)

cursiv_package_language(fusiontables)
