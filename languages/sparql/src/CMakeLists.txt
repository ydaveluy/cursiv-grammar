# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `sparqlGenerated` (the compiled library) and `sparql_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID sparql
    GRAMMAR    sparql/core/SparqlGrammar.hpp
    NAMESPACE  sparql::Sparql
    EXTENSIONS .rq
    OUT        sparql/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sparql/core/*.cpp)
add_library(sparqlCore ${_core})
target_link_libraries(sparqlCore PUBLIC sparqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sparql/cli/*.cpp)
add_executable(sparql_cli ${_cli})
target_link_libraries(sparql_cli
                      PRIVATE sparqlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sparql/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sparql/lsp/main.cpp)
add_library(sparqlLsp ${_lsp})
target_link_libraries(sparqlLsp PUBLIC sparqlCore cursiv::lsp)
add_executable(sparql_lsp sparql/lsp/main.cpp)
target_link_libraries(sparql_lsp PRIVATE sparqlLsp)

cursiv_add_benchmark(sparql SOURCES sparql/bench/main.cpp)

cursiv_package_language(sparql)
