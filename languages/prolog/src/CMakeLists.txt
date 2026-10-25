# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `prologGenerated` (the compiled library) and `prolog_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID prolog
    GRAMMAR    prolog/core/PrologGrammar.hpp
    NAMESPACE  prolog::Prolog
    EXTENSIONS .pl
    OUT        prolog/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/prolog/core/*.cpp)
add_library(prologCore ${_core})
target_link_libraries(prologCore PUBLIC prologGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/prolog/cli/*.cpp)
add_executable(prolog_cli ${_cli})
target_link_libraries(prolog_cli
                      PRIVATE prologCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/prolog/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/prolog/lsp/main.cpp)
add_library(prologLsp ${_lsp})
target_link_libraries(prologLsp PUBLIC prologCore cursiv::lsp)
add_executable(prolog_lsp prolog/lsp/main.cpp)
target_link_libraries(prolog_lsp PRIVATE prologLsp)

cursiv_add_benchmark(prolog SOURCES prolog/bench/main.cpp)

cursiv_package_language(prolog)
