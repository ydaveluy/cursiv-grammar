# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pfGenerated` (the compiled library) and `pf_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pf
    GRAMMAR    pf/core/PfGrammar.hpp
    NAMESPACE  pf::Pf
    EXTENSIONS .pf
    OUT        pf/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pf/core/*.cpp)
add_library(pfCore ${_core})
target_link_libraries(pfCore PUBLIC pfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pf/cli/*.cpp)
add_executable(pf_cli ${_cli})
target_link_libraries(pf_cli
                      PRIVATE pfCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pf/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pf/lsp/main.cpp)
add_library(pfLsp ${_lsp})
target_link_libraries(pfLsp PUBLIC pfCore cursiv::lsp)
add_executable(pf_lsp pf/lsp/main.cpp)
target_link_libraries(pf_lsp PRIVATE pfLsp)

cursiv_add_benchmark(pf SOURCES pf/bench/main.cpp)

cursiv_package_language(pf)
