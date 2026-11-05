# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID wren
    GRAMMAR    wren/WrenGrammar.hpp
    NAMESPACE  wren::Wren
    EXTENSIONS .wren
    OUT        wren/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(wrenCore ${_core})
target_link_libraries(wrenCore PUBLIC wrenGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(wren_cli cli.cpp)
  target_include_directories(wren_cli PRIVATE cli)
  target_link_libraries(wren_cli PRIVATE wrenCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(wrenCli ${_cli})
    target_include_directories(wrenCli PUBLIC cli)
    target_link_libraries(wrenCli PUBLIC wrenCore cursiv::cli)
    target_link_libraries(wren_cli PRIVATE wrenCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(wrenLsp ${_lsp})
  target_include_directories(wrenLsp PUBLIC lsp)
  target_link_libraries(wrenLsp PUBLIC wrenCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(wren_lsp lsp.cpp)
    target_link_libraries(wren_lsp PRIVATE wrenLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET wrenLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(wren SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(wren)
