# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `fenGenerated` (the compiled library) and `fen_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID fen
    GRAMMAR    fen/core/FenGrammar.hpp
    NAMESPACE  fen::Fen
    EXTENSIONS .fen
    OUT        fen/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fen/core/*.cpp)
add_library(fenCore ${_core})
target_link_libraries(fenCore PUBLIC fenGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fen/cli/*.cpp)
add_executable(fen_cli ${_cli})
target_link_libraries(fen_cli
                      PRIVATE fenCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fen/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/fen/lsp/main.cpp)
add_library(fenLsp ${_lsp})
target_link_libraries(fenLsp PUBLIC fenCore cursiv::lsp)
add_executable(fen_lsp fen/lsp/main.cpp)
target_link_libraries(fen_lsp PRIVATE fenLsp)

cursiv_add_benchmark(fen SOURCES fen/bench/main.cpp)

cursiv_package_language(fen)
