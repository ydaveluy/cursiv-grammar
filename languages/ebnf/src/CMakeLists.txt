# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `ebnfGenerated` (the compiled library) and `ebnf_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID ebnf
    GRAMMAR    ebnf/core/EbnfGrammar.hpp
    NAMESPACE  ebnf::Ebnf
    EXTENSIONS .ebnf
    OUT        ebnf/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ebnf/core/*.cpp)
add_library(ebnfCore ${_core})
target_link_libraries(ebnfCore PUBLIC ebnfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ebnf/cli/*.cpp)
add_executable(ebnf_cli ${_cli})
target_link_libraries(ebnf_cli
                      PRIVATE ebnfCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ebnf/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ebnf/lsp/main.cpp)
add_library(ebnfLsp ${_lsp})
target_link_libraries(ebnfLsp PUBLIC ebnfCore cursiv::lsp)
add_executable(ebnf_lsp ebnf/lsp/main.cpp)
target_link_libraries(ebnf_lsp PRIVATE ebnfLsp)

cursiv_add_benchmark(ebnf SOURCES ebnf/bench/main.cpp)

cursiv_package_language(ebnf)
