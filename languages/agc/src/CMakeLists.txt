# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `agcGenerated` (the compiled library) and `agc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID agc
    GRAMMAR    agc/core/AgcGrammar.hpp
    NAMESPACE  agc::Agc
    EXTENSIONS .agc
    OUT        agc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/agc/core/*.cpp)
add_library(agcCore ${_core})
target_link_libraries(agcCore PUBLIC agcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/agc/cli/*.cpp)
add_executable(agc_cli ${_cli})
target_link_libraries(agc_cli
                      PRIVATE agcCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/agc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/agc/lsp/main.cpp)
add_library(agcLsp ${_lsp})
target_link_libraries(agcLsp PUBLIC agcCore cursiv::lsp)
add_executable(agc_lsp agc/lsp/main.cpp)
target_link_libraries(agc_lsp PRIVATE agcLsp)

cursiv_add_benchmark(agc SOURCES agc/bench/main.cpp)

cursiv_package_language(agc)
