# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `cypherGenerated` (the compiled library) and `cypher_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID cypher
    GRAMMAR    cypher/core/CypherGrammar.hpp
    NAMESPACE  cypher::Cypher
    EXTENSIONS .cypher
    OUT        cypher/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cypher/core/*.cpp)
add_library(cypherCore ${_core})
target_link_libraries(cypherCore PUBLIC cypherGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cypher/cli/*.cpp)
add_executable(cypher_cli ${_cli})
target_link_libraries(cypher_cli
                      PRIVATE cypherCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cypher/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cypher/lsp/main.cpp)
add_library(cypherLsp ${_lsp})
target_link_libraries(cypherLsp PUBLIC cypherCore cursiv::lsp)
add_executable(cypher_lsp cypher/lsp/main.cpp)
target_link_libraries(cypher_lsp PRIVATE cypherLsp)

cursiv_add_benchmark(cypher SOURCES cypher/bench/main.cpp)

cursiv_package_language(cypher)
