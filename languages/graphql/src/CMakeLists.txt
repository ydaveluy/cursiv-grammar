# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `graphqlGenerated` (the compiled library) and `graphql_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID graphql
    GRAMMAR    graphql/core/GraphqlGrammar.hpp
    NAMESPACE  graphql::Graphql
    EXTENSIONS .graphql
    OUT        graphql/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphql/core/*.cpp)
add_library(graphqlCore ${_core})
target_link_libraries(graphqlCore PUBLIC graphqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphql/cli/*.cpp)
add_executable(graphql_cli ${_cli})
target_link_libraries(graphql_cli
                      PRIVATE graphqlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphql/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/graphql/lsp/main.cpp)
add_library(graphqlLsp ${_lsp})
target_link_libraries(graphqlLsp PUBLIC graphqlCore cursiv::lsp)
add_executable(graphql_lsp graphql/lsp/main.cpp)
target_link_libraries(graphql_lsp PRIVATE graphqlLsp)

cursiv_add_benchmark(graphql SOURCES graphql/bench/main.cpp)

cursiv_package_language(graphql)
