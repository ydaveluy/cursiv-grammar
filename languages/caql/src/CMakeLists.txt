# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `caqlGenerated` (the compiled library) and `caql_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID caql
    GRAMMAR    caql/core/CaqlGrammar.hpp
    NAMESPACE  caql::Caql
    EXTENSIONS .caql
    OUT        caql/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/caql/core/*.cpp)
add_library(caqlCore ${_core})
target_link_libraries(caqlCore PUBLIC caqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/caql/cli/*.cpp)
add_executable(caql_cli ${_cli})
target_link_libraries(caql_cli
                      PRIVATE caqlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/caql/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/caql/lsp/main.cpp)
add_library(caqlLsp ${_lsp})
target_link_libraries(caqlLsp PUBLIC caqlCore cursiv::lsp)
add_executable(caql_lsp caql/lsp/main.cpp)
target_link_libraries(caql_lsp PRIVATE caqlLsp)

cursiv_add_benchmark(caql SOURCES caql/bench/main.cpp)

cursiv_package_language(caql)
