# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `promqlGenerated` (the compiled library) and `promql_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID promql
    GRAMMAR    promql/core/PromqlGrammar.hpp
    NAMESPACE  promql::Promql
    EXTENSIONS .promql
    OUT        promql/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/promql/core/*.cpp)
add_library(promqlCore ${_core})
target_link_libraries(promqlCore PUBLIC promqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/promql/cli/*.cpp)
add_executable(promql_cli ${_cli})
target_link_libraries(promql_cli
                      PRIVATE promqlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/promql/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/promql/lsp/main.cpp)
add_library(promqlLsp ${_lsp})
target_link_libraries(promqlLsp PUBLIC promqlCore cursiv::lsp)
add_executable(promql_lsp promql/lsp/main.cpp)
target_link_libraries(promql_lsp PRIVATE promqlLsp)

cursiv_add_benchmark(promql SOURCES promql/bench/main.cpp)

cursiv_package_language(promql)
