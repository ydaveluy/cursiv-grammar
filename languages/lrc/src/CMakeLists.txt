# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID lrc
    GRAMMAR    lrc/LrcGrammar.hpp
    NAMESPACE  lrc::Lrc
    EXTENSIONS .lrc
    OUT        lrc/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(lrcCore ${_core})
target_link_libraries(lrcCore PUBLIC lrcGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(lrc_cli cli.cpp)
  target_include_directories(lrc_cli PRIVATE cli)
  target_link_libraries(lrc_cli PRIVATE lrcCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(lrcCli ${_cli})
    target_include_directories(lrcCli PUBLIC cli)
    target_link_libraries(lrcCli PUBLIC lrcCore cursiv::cli)
    target_link_libraries(lrc_cli PRIVATE lrcCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(lrcLsp ${_lsp})
  target_include_directories(lrcLsp PUBLIC lsp)
  target_link_libraries(lrcLsp PUBLIC lrcCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(lrc_lsp lsp.cpp)
    target_link_libraries(lrc_lsp PRIVATE lrcLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET lrcLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(lrc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(lrc)
