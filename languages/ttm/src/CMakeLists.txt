# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `ttmGenerated` (the compiled library) and `ttm_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID ttm
    GRAMMAR    ttm/core/TtmGrammar.hpp
    NAMESPACE  ttm::Ttm
    EXTENSIONS .ttm
    OUT        ttm/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ttm/core/*.cpp)
add_library(ttmCore ${_core})
target_link_libraries(ttmCore PUBLIC ttmGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ttm/cli/*.cpp)
add_executable(ttm_cli ${_cli})
target_link_libraries(ttm_cli
                      PRIVATE ttmCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ttm/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ttm/lsp/main.cpp)
add_library(ttmLsp ${_lsp})
target_link_libraries(ttmLsp PUBLIC ttmCore cursiv::lsp)
add_executable(ttm_lsp ttm/lsp/main.cpp)
target_link_libraries(ttm_lsp PRIVATE ttmLsp)

cursiv_add_benchmark(ttm SOURCES ttm/bench/main.cpp)

cursiv_package_language(ttm)
