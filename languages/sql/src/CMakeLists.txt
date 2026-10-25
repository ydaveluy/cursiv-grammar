# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `sqliteGenerated` (the compiled library) and `sqlite_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID sqlite
    GRAMMAR    sqlite/core/SqliteGrammar.hpp
    NAMESPACE  sqlite::Sqlite
    EXTENSIONS .sql
    OUT        sqlite/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/core/*.cpp)
add_library(sqliteCore ${_core})
target_link_libraries(sqliteCore PUBLIC sqliteGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/cli/*.cpp)
add_executable(sqlite_cli ${_cli})
target_link_libraries(sqlite_cli
                      PRIVATE sqliteCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/lsp/main.cpp)
add_library(sqliteLsp ${_lsp})
target_link_libraries(sqliteLsp PUBLIC sqliteCore cursiv::lsp)
add_executable(sqlite_lsp sqlite/lsp/main.cpp)
target_link_libraries(sqlite_lsp PRIVATE sqliteLsp)

cursiv_add_benchmark(sqlite SOURCES sqlite/bench/main.cpp)

cursiv_package_language(sqlite)
