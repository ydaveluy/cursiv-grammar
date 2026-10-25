# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `guitartabGenerated` (the compiled library) and `guitartab_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID guitartab
    GRAMMAR    guitartab/core/GuitartabGrammar.hpp
    NAMESPACE  guitartab::Guitartab
    EXTENSIONS .guitartab
    OUT        guitartab/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/guitartab/core/*.cpp)
add_library(guitartabCore ${_core})
target_link_libraries(guitartabCore PUBLIC guitartabGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/guitartab/cli/*.cpp)
add_executable(guitartab_cli ${_cli})
target_link_libraries(guitartab_cli
                      PRIVATE guitartabCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/guitartab/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/guitartab/lsp/main.cpp)
add_library(guitartabLsp ${_lsp})
target_link_libraries(guitartabLsp PUBLIC guitartabCore cursiv::lsp)
add_executable(guitartab_lsp guitartab/lsp/main.cpp)
target_link_libraries(guitartab_lsp PRIVATE guitartabLsp)

cursiv_add_benchmark(guitartab SOURCES guitartab/bench/main.cpp)

cursiv_package_language(guitartab)
