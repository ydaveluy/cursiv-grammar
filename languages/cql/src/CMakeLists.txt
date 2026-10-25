# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `cqlGenerated` (the compiled library) and `cql_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID cql
    GRAMMAR    cql/core/CqlGrammar.hpp
    NAMESPACE  cql::Cql
    EXTENSIONS .cql
    OUT        cql/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cql/core/*.cpp)
add_library(cqlCore ${_core})
target_link_libraries(cqlCore PUBLIC cqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cql/cli/*.cpp)
add_executable(cql_cli ${_cli})
target_link_libraries(cql_cli
                      PRIVATE cqlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cql/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cql/lsp/main.cpp)
add_library(cqlLsp ${_lsp})
target_link_libraries(cqlLsp PUBLIC cqlCore cursiv::lsp)
add_executable(cql_lsp cql/lsp/main.cpp)
target_link_libraries(cql_lsp PRIVATE cqlLsp)

cursiv_add_benchmark(cql SOURCES cql/bench/main.cpp)

cursiv_package_language(cql)
