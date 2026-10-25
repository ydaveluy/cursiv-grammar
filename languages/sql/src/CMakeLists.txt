# Generate the parser and its language module.
cursiv_generate_language(ID sqlite
    GRAMMAR    sqlite/core/SqliteGrammar.hpp
    NAMESPACE  sqlite::Sqlite
    EXTENSIONS .sql
    OUT        sqlite/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/core/*.cpp)
add_library(sqliteCore ${_core})
target_link_libraries(sqliteCore PUBLIC sqliteGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/cli/*.cpp)
if(_cli)
  add_executable(sqlite_cli ${_cli})
  target_link_libraries(sqlite_cli
                        PRIVATE sqliteCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sqlite/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/lsp/main.cpp)
  add_library(sqliteLsp ${_lsp})
  target_link_libraries(sqliteLsp PUBLIC sqliteCore cursiv::lsp)
  add_executable(sqlite_lsp sqlite/lsp/main.cpp)
  target_link_libraries(sqlite_lsp PRIVATE sqliteLsp)
endif()

if(TARGET sqliteLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sqlite/bench/main.cpp")
  cursiv_add_benchmark(sqlite
      SOURCES sqlite/bench/main.cpp)
endif()

cursiv_package_language(sqlite)
