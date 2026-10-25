# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `luaGenerated` (the compiled library) and `lua_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID lua
    GRAMMAR    lua/core/LuaGrammar.hpp
    NAMESPACE  lua::Lua
    EXTENSIONS .lua
    OUT        lua/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lua/core/*.cpp)
add_library(luaCore ${_core})
target_link_libraries(luaCore PUBLIC luaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lua/cli/*.cpp)
add_executable(lua_cli ${_cli})
target_link_libraries(lua_cli
                      PRIVATE luaCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lua/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lua/lsp/main.cpp)
add_library(luaLsp ${_lsp})
target_link_libraries(luaLsp PUBLIC luaCore cursiv::lsp)
add_executable(lua_lsp lua/lsp/main.cpp)
target_link_libraries(lua_lsp PRIVATE luaLsp)

cursiv_add_benchmark(lua SOURCES lua/bench/main.cpp)

cursiv_package_language(lua)
