# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pdnGenerated` (the compiled library) and `pdn_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pdn
    GRAMMAR    pdn/core/PdnGrammar.hpp
    NAMESPACE  pdn::Pdn
    EXTENSIONS .pdn
    OUT        pdn/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pdn/core/*.cpp)
add_library(pdnCore ${_core})
target_link_libraries(pdnCore PUBLIC pdnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pdn/cli/*.cpp)
add_executable(pdn_cli ${_cli})
target_link_libraries(pdn_cli
                      PRIVATE pdnCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pdn/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pdn/lsp/main.cpp)
add_library(pdnLsp ${_lsp})
target_link_libraries(pdnLsp PUBLIC pdnCore cursiv::lsp)
add_executable(pdn_lsp pdn/lsp/main.cpp)
target_link_libraries(pdn_lsp PRIVATE pdnLsp)

cursiv_add_benchmark(pdn SOURCES pdn/bench/main.cpp)

cursiv_package_language(pdn)
