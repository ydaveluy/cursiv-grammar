# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `wlnGenerated` (the compiled library) and `wln_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID wln
    GRAMMAR    wln/core/WlnGrammar.hpp
    NAMESPACE  wln::Wln
    EXTENSIONS .wln
    OUT        wln/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wln/core/*.cpp)
add_library(wlnCore ${_core})
target_link_libraries(wlnCore PUBLIC wlnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wln/cli/*.cpp)
add_executable(wln_cli ${_cli})
target_link_libraries(wln_cli
                      PRIVATE wlnCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wln/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/wln/lsp/main.cpp)
add_library(wlnLsp ${_lsp})
target_link_libraries(wlnLsp PUBLIC wlnCore cursiv::lsp)
add_executable(wln_lsp wln/lsp/main.cpp)
target_link_libraries(wln_lsp PRIVATE wlnLsp)

cursiv_add_benchmark(wln SOURCES wln/bench/main.cpp)

cursiv_package_language(wln)
