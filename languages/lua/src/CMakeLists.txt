# Generate the parser and its language module.
cursiv_generate_language(ID lua
    GRAMMAR    lua/core/LuaGrammar.hpp
    NAMESPACE  lua::Lua
    EXTENSIONS .lua
    OUT        lua/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lua/core/*.cpp)
add_library(luaCore ${_core})
target_link_libraries(luaCore PUBLIC luaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lua/cli/*.cpp)
if(_cli)
  add_executable(lua_cli ${_cli})
  target_link_libraries(lua_cli
                        PRIVATE luaCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lua/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lua/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lua/lsp/main.cpp)
  add_library(luaLsp ${_lsp})
  target_link_libraries(luaLsp PUBLIC luaCore cursiv::lsp)
  add_executable(lua_lsp lua/lsp/main.cpp)
  target_link_libraries(lua_lsp PRIVATE luaLsp)
endif()

if(TARGET luaLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lua/bench/main.cpp")
  cursiv_add_benchmark(lua
      SOURCES lua/bench/main.cpp)
endif()

cursiv_package_language(lua)
