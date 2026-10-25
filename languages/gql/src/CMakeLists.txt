# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `gqlGenerated` (the compiled library) and `gql_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID gql
    GRAMMAR    gql/core/GqlGrammar.hpp
    NAMESPACE  gql::Gql
    EXTENSIONS .gql
    OUT        gql/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gql/core/*.cpp)
add_library(gqlCore ${_core})
target_link_libraries(gqlCore PUBLIC gqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gql/cli/*.cpp)
add_executable(gql_cli ${_cli})
target_link_libraries(gql_cli
                      PRIVATE gqlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gql/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gql/lsp/main.cpp)
add_library(gqlLsp ${_lsp})
target_link_libraries(gqlLsp PUBLIC gqlCore cursiv::lsp)
add_executable(gql_lsp gql/lsp/main.cpp)
target_link_libraries(gql_lsp PRIVATE gqlLsp)

cursiv_add_benchmark(gql SOURCES gql/bench/main.cpp)

cursiv_package_language(gql)
