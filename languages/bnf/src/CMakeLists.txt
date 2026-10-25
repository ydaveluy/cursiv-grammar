# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `bnfGenerated` (the compiled library) and `bnf_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID bnf
    GRAMMAR    bnf/core/BnfGrammar.hpp
    NAMESPACE  bnf::Bnf
    EXTENSIONS .bnf
    OUT        bnf/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bnf/core/*.cpp)
add_library(bnfCore ${_core})
target_link_libraries(bnfCore PUBLIC bnfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bnf/cli/*.cpp)
add_executable(bnf_cli ${_cli})
target_link_libraries(bnf_cli
                      PRIVATE bnfCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bnf/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bnf/lsp/main.cpp)
add_library(bnfLsp ${_lsp})
target_link_libraries(bnfLsp PUBLIC bnfCore cursiv::lsp)
add_executable(bnf_lsp bnf/lsp/main.cpp)
target_link_libraries(bnf_lsp PRIVATE bnfLsp)

cursiv_add_benchmark(bnf SOURCES bnf/bench/main.cpp)

cursiv_package_language(bnf)
