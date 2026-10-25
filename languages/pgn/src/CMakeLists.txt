# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pgnGenerated` (the compiled library) and `pgn_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pgn
    GRAMMAR    pgn/core/PgnGrammar.hpp
    NAMESPACE  pgn::Pgn
    EXTENSIONS .pgn
    OUT        pgn/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pgn/core/*.cpp)
add_library(pgnCore ${_core})
target_link_libraries(pgnCore PUBLIC pgnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pgn/cli/*.cpp)
add_executable(pgn_cli ${_cli})
target_link_libraries(pgn_cli
                      PRIVATE pgnCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pgn/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pgn/lsp/main.cpp)
add_library(pgnLsp ${_lsp})
target_link_libraries(pgnLsp PUBLIC pgnCore cursiv::lsp)
add_executable(pgn_lsp pgn/lsp/main.cpp)
target_link_libraries(pgn_lsp PRIVATE pgnLsp)

cursiv_add_benchmark(pgn SOURCES pgn/bench/main.cpp)

cursiv_package_language(pgn)
