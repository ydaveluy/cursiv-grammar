# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `propcalcGenerated` (the compiled library) and `propcalc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID propcalc
    GRAMMAR    propcalc/core/PropcalcGrammar.hpp
    NAMESPACE  propcalc::Propcalc
    EXTENSIONS .propcalc
    OUT        propcalc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/propcalc/core/*.cpp)
add_library(propcalcCore ${_core})
target_link_libraries(propcalcCore PUBLIC propcalcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/propcalc/cli/*.cpp)
add_executable(propcalc_cli ${_cli})
target_link_libraries(propcalc_cli
                      PRIVATE propcalcCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/propcalc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/propcalc/lsp/main.cpp)
add_library(propcalcLsp ${_lsp})
target_link_libraries(propcalcLsp PUBLIC propcalcCore cursiv::lsp)
add_executable(propcalc_lsp propcalc/lsp/main.cpp)
target_link_libraries(propcalc_lsp PRIVATE propcalcLsp)

cursiv_add_benchmark(propcalc SOURCES propcalc/bench/main.cpp)

cursiv_package_language(propcalc)
