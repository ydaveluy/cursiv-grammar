# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID karel
    GRAMMAR    karel/KarelGrammar.hpp
    NAMESPACE  karel::Karel
    EXTENSIONS .karel
    OUT        karel/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(karelCore ${_core})
target_link_libraries(karelCore PUBLIC karelGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(karel_cli cli.cpp)
  target_include_directories(karel_cli PRIVATE cli)
  target_link_libraries(karel_cli PRIVATE karelCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(karelCli ${_cli})
    target_include_directories(karelCli PUBLIC cli)
    target_link_libraries(karelCli PUBLIC karelCore cursiv::cli)
    target_link_libraries(karel_cli PRIVATE karelCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(karelLsp ${_lsp})
  target_include_directories(karelLsp PUBLIC lsp)
  target_link_libraries(karelLsp PUBLIC karelCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(karel_lsp lsp.cpp)
    target_link_libraries(karel_lsp PRIVATE karelLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET karelLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(karel SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(karel)
