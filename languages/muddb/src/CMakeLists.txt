# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `muddbGenerated` (the compiled library) and `muddb_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID muddb
    GRAMMAR    muddb/core/MuddbGrammar.hpp
    NAMESPACE  muddb::Muddb
    EXTENSIONS .muddb
    OUT        muddb/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/muddb/core/*.cpp)
add_library(muddbCore ${_core})
target_link_libraries(muddbCore PUBLIC muddbGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/muddb/cli/*.cpp)
add_executable(muddb_cli ${_cli})
target_link_libraries(muddb_cli
                      PRIVATE muddbCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/muddb/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/muddb/lsp/main.cpp)
add_library(muddbLsp ${_lsp})
target_link_libraries(muddbLsp PUBLIC muddbCore cursiv::lsp)
add_executable(muddb_lsp muddb/lsp/main.cpp)
target_link_libraries(muddb_lsp PRIVATE muddbLsp)

cursiv_add_benchmark(muddb SOURCES muddb/bench/main.cpp)

cursiv_package_language(muddb)
