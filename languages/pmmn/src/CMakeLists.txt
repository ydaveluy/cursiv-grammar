# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pmmnGenerated` (the compiled library) and `pmmn_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pmmn
    GRAMMAR    pmmn/core/PmmnGrammar.hpp
    NAMESPACE  pmmn::Pmmn
    EXTENSIONS .pmmn
    OUT        pmmn/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pmmn/core/*.cpp)
add_library(pmmnCore ${_core})
target_link_libraries(pmmnCore PUBLIC pmmnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pmmn/cli/*.cpp)
add_executable(pmmn_cli ${_cli})
target_link_libraries(pmmn_cli
                      PRIVATE pmmnCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pmmn/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pmmn/lsp/main.cpp)
add_library(pmmnLsp ${_lsp})
target_link_libraries(pmmnLsp PUBLIC pmmnCore cursiv::lsp)
add_executable(pmmn_lsp pmmn/lsp/main.cpp)
target_link_libraries(pmmn_lsp PRIVATE pmmnLsp)

cursiv_add_benchmark(pmmn SOURCES pmmn/bench/main.cpp)

cursiv_package_language(pmmn)
