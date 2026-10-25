# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `suokifGenerated` (the compiled library) and `suokif_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID suokif
    GRAMMAR    suokif/core/SuokifGrammar.hpp
    NAMESPACE  suokif::Suokif
    EXTENSIONS .kif
    OUT        suokif/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/suokif/core/*.cpp)
add_library(suokifCore ${_core})
target_link_libraries(suokifCore PUBLIC suokifGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/suokif/cli/*.cpp)
add_executable(suokif_cli ${_cli})
target_link_libraries(suokif_cli
                      PRIVATE suokifCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/suokif/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/suokif/lsp/main.cpp)
add_library(suokifLsp ${_lsp})
target_link_libraries(suokifLsp PUBLIC suokifCore cursiv::lsp)
add_executable(suokif_lsp suokif/lsp/main.cpp)
target_link_libraries(suokif_lsp PRIVATE suokifLsp)

cursiv_add_benchmark(suokif SOURCES suokif/bench/main.cpp)

cursiv_package_language(suokif)
