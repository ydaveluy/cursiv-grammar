# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `codeqlGenerated` (the compiled library) and `codeql_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID codeql
    GRAMMAR    codeql/core/CodeqlGrammar.hpp
    NAMESPACE  codeql::Codeql
    EXTENSIONS .ql .qll
    OUT        codeql/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/codeql/core/*.cpp)
add_library(codeqlCore ${_core})
target_link_libraries(codeqlCore PUBLIC codeqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/codeql/cli/*.cpp)
add_executable(codeql_cli ${_cli})
target_link_libraries(codeql_cli
                      PRIVATE codeqlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/codeql/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/codeql/lsp/main.cpp)
add_library(codeqlLsp ${_lsp})
target_link_libraries(codeqlLsp PUBLIC codeqlCore cursiv::lsp)
add_executable(codeql_lsp codeql/lsp/main.cpp)
target_link_libraries(codeql_lsp PRIVATE codeqlLsp)

cursiv_add_benchmark(codeql SOURCES codeql/bench/main.cpp)

cursiv_package_language(codeql)
