# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID creole
    GRAMMAR    creole/CreoleGrammar.hpp
    NAMESPACE  creole::Creole
    EXTENSIONS .creole
    OUT        creole/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(creoleCore ${_core})
target_link_libraries(creoleCore PUBLIC creoleGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(creole_cli cli.cpp)
  target_include_directories(creole_cli PRIVATE cli)
  target_link_libraries(creole_cli PRIVATE creoleCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(creoleCli ${_cli})
    target_include_directories(creoleCli PUBLIC cli)
    target_link_libraries(creoleCli PUBLIC creoleCore cursiv::cli)
    target_link_libraries(creole_cli PRIVATE creoleCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(creoleLsp ${_lsp})
  target_include_directories(creoleLsp PUBLIC lsp)
  target_link_libraries(creoleLsp PUBLIC creoleCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(creole_lsp lsp.cpp)
    target_link_libraries(creole_lsp PRIVATE creoleLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET creoleLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(creole SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(creole)
