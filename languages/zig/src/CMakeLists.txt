# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID zig
    GRAMMAR    zig/ZigGrammar.hpp
    NAMESPACE  zig::Zig
    EXTENSIONS .zig
    OUT        zig/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(zigCore ${_core})
target_link_libraries(zigCore PUBLIC zigGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(zig_cli cli.cpp)
  target_include_directories(zig_cli PRIVATE cli)
  target_link_libraries(zig_cli PRIVATE zigCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(zigCli ${_cli})
    target_include_directories(zigCli PUBLIC cli)
    target_link_libraries(zigCli PUBLIC zigCore cursiv::cli)
    target_link_libraries(zig_cli PRIVATE zigCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(zigLsp ${_lsp})
  target_include_directories(zigLsp PUBLIC lsp)
  target_link_libraries(zigLsp PUBLIC zigCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(zig_lsp lsp.cpp)
    target_link_libraries(zig_lsp PRIVATE zigLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET zigLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(zig SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(zig)
