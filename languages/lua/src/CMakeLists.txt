# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID lua
    GRAMMAR    lua/LuaGrammar.hpp
    NAMESPACE  lua::Lua
    EXTENSIONS .lua
    OUT        lua/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(luaCore ${_core})
target_link_libraries(luaCore PUBLIC luaGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(lua_cli cli.cpp)
  target_include_directories(lua_cli PRIVATE cli)
  target_link_libraries(lua_cli PRIVATE luaCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(luaCli ${_cli})
    target_include_directories(luaCli PUBLIC cli)
    target_link_libraries(luaCli PUBLIC luaCore cursiv::cli)
    target_link_libraries(lua_cli PRIVATE luaCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(luaLsp ${_lsp})
  target_include_directories(luaLsp PUBLIC lsp)
  target_link_libraries(luaLsp PUBLIC luaCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(lua_lsp lsp.cpp)
    target_link_libraries(lua_lsp PRIVATE luaLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET luaLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(lua SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(lua)
