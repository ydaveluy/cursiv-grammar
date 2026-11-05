# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID sqlite
    GRAMMAR    sqlite/SqliteGrammar.hpp
    NAMESPACE  sqlite::Sqlite
    EXTENSIONS .sql
    OUT        sqlite/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(sqliteCore ${_core})
target_link_libraries(sqliteCore PUBLIC sqliteGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(sqlite_cli cli.cpp)
  target_include_directories(sqlite_cli PRIVATE cli)
  target_link_libraries(sqlite_cli PRIVATE sqliteCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(sqliteCli ${_cli})
    target_include_directories(sqliteCli PUBLIC cli)
    target_link_libraries(sqliteCli PUBLIC sqliteCore cursiv::cli)
    target_link_libraries(sqlite_cli PRIVATE sqliteCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(sqliteLsp ${_lsp})
  target_include_directories(sqliteLsp PUBLIC lsp)
  target_link_libraries(sqliteLsp PUBLIC sqliteCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(sqlite_lsp lsp.cpp)
    target_link_libraries(sqlite_lsp PRIVATE sqliteLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET sqliteLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(sqlite SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(sqlite)
