# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `sexpressionGenerated` (the compiled library) and `sexpression_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID sexpression
    GRAMMAR    sexpression/core/SexpressionGrammar.hpp
    NAMESPACE  sexpression::Sexpression
    EXTENSIONS .sexpression
    OUT        sexpression/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sexpression/core/*.cpp)
add_library(sexpressionCore ${_core})
target_link_libraries(sexpressionCore PUBLIC sexpressionGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sexpression/cli/*.cpp)
add_executable(sexpression_cli ${_cli})
target_link_libraries(sexpression_cli
                      PRIVATE sexpressionCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sexpression/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sexpression/lsp/main.cpp)
add_library(sexpressionLsp ${_lsp})
target_link_libraries(sexpressionLsp PUBLIC sexpressionCore cursiv::lsp)
add_executable(sexpression_lsp sexpression/lsp/main.cpp)
target_link_libraries(sexpression_lsp PRIVATE sexpressionLsp)

cursiv_add_benchmark(sexpression SOURCES sexpression/bench/main.cpp)

cursiv_package_language(sexpression)
