# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID action
    GRAMMAR    action/ActionGrammar.hpp
    NAMESPACE  action::Action
    EXTENSIONS .act
    OUT        action/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(actionCore ${_core})
target_link_libraries(actionCore PUBLIC actionGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(action_cli cli.cpp)
  target_include_directories(action_cli PRIVATE cli)
  target_link_libraries(action_cli PRIVATE actionCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(actionCli ${_cli})
    target_include_directories(actionCli PUBLIC cli)
    target_link_libraries(actionCli PUBLIC actionCore cursiv::cli)
    target_link_libraries(action_cli PRIVATE actionCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(actionLsp ${_lsp})
  target_include_directories(actionLsp PUBLIC lsp)
  target_link_libraries(actionLsp PUBLIC actionCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(action_lsp lsp.cpp)
    target_link_libraries(action_lsp PRIVATE actionLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET actionLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(action SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(action)
