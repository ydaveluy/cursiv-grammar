# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `turingGenerated` (the compiled library) and `turing_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID turing
    GRAMMAR    turing/core/TuringGrammar.hpp
    NAMESPACE  turing::Turing
    EXTENSIONS .t
    OUT        turing/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turing/core/*.cpp)
add_library(turingCore ${_core})
target_link_libraries(turingCore PUBLIC turingGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turing/cli/*.cpp)
add_executable(turing_cli ${_cli})
target_link_libraries(turing_cli
                      PRIVATE turingCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turing/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/turing/lsp/main.cpp)
add_library(turingLsp ${_lsp})
target_link_libraries(turingLsp PUBLIC turingCore cursiv::lsp)
add_executable(turing_lsp turing/lsp/main.cpp)
target_link_libraries(turing_lsp PRIVATE turingLsp)

cursiv_add_benchmark(turing SOURCES turing/bench/main.cpp)

cursiv_package_language(turing)
