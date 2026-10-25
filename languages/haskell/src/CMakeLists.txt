# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `haskellGenerated` (the compiled library) and `haskell_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID haskell
    GRAMMAR    haskell/core/HaskellGrammar.hpp
    NAMESPACE  haskell::Haskell
    EXTENSIONS .hs
    OUT        haskell/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/haskell/core/*.cpp)
add_library(haskellCore ${_core})
target_link_libraries(haskellCore PUBLIC haskellGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/haskell/cli/*.cpp)
add_executable(haskell_cli ${_cli})
target_link_libraries(haskell_cli
                      PRIVATE haskellCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/haskell/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/haskell/lsp/main.cpp)
add_library(haskellLsp ${_lsp})
target_link_libraries(haskellLsp PUBLIC haskellCore cursiv::lsp)
add_executable(haskell_lsp haskell/lsp/main.cpp)
target_link_libraries(haskell_lsp PRIVATE haskellLsp)

cursiv_add_benchmark(haskell SOURCES haskell/bench/main.cpp)

cursiv_package_language(haskell)
