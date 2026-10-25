# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `antlrGenerated` (the compiled library) and `antlr_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID antlr
    GRAMMAR    antlr/core/AntlrGrammar.hpp
    NAMESPACE  antlr::Antlr
    EXTENSIONS .g4
    OUT        antlr/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/antlr/core/*.cpp)
add_library(antlrCore ${_core})
target_link_libraries(antlrCore PUBLIC antlrGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/antlr/cli/*.cpp)
add_executable(antlr_cli ${_cli})
target_link_libraries(antlr_cli
                      PRIVATE antlrCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/antlr/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/antlr/lsp/main.cpp)
add_library(antlrLsp ${_lsp})
target_link_libraries(antlrLsp PUBLIC antlrCore cursiv::lsp)
add_executable(antlr_lsp antlr/lsp/main.cpp)
target_link_libraries(antlr_lsp PRIVATE antlrLsp)

cursiv_add_benchmark(antlr SOURCES antlr/bench/main.cpp)

cursiv_package_language(antlr)
