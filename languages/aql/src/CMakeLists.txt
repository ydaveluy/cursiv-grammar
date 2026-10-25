# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `aqlGenerated` (the compiled library) and `aql_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID aql
    GRAMMAR    aql/core/AqlGrammar.hpp
    NAMESPACE  aql::Aql
    EXTENSIONS .aql
    OUT        aql/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aql/core/*.cpp)
add_library(aqlCore ${_core})
target_link_libraries(aqlCore PUBLIC aqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aql/cli/*.cpp)
add_executable(aql_cli ${_cli})
target_link_libraries(aql_cli
                      PRIVATE aqlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aql/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/aql/lsp/main.cpp)
add_library(aqlLsp ${_lsp})
target_link_libraries(aqlLsp PUBLIC aqlCore cursiv::lsp)
add_executable(aql_lsp aql/lsp/main.cpp)
target_link_libraries(aql_lsp PRIVATE aqlLsp)

cursiv_add_benchmark(aql SOURCES aql/bench/main.cpp)

cursiv_package_language(aql)
