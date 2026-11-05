# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID gdscript
    GRAMMAR    gdscript/GdscriptGrammar.hpp
    NAMESPACE  gdscript::Gdscript
    EXTENSIONS .gd
    OUT        gdscript/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(gdscriptCore ${_core})
target_link_libraries(gdscriptCore PUBLIC gdscriptGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(gdscript_cli cli.cpp)
  target_include_directories(gdscript_cli PRIVATE cli)
  target_link_libraries(gdscript_cli PRIVATE gdscriptCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(gdscriptCli ${_cli})
    target_include_directories(gdscriptCli PUBLIC cli)
    target_link_libraries(gdscriptCli PUBLIC gdscriptCore cursiv::cli)
    target_link_libraries(gdscript_cli PRIVATE gdscriptCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(gdscriptLsp ${_lsp})
  target_include_directories(gdscriptLsp PUBLIC lsp)
  target_link_libraries(gdscriptLsp PUBLIC gdscriptCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(gdscript_lsp lsp.cpp)
    target_link_libraries(gdscript_lsp PRIVATE gdscriptLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET gdscriptLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(gdscript SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(gdscript)
