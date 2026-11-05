# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID tinyosnesc
    GRAMMAR    tinyosnesc/TinyosNescGrammar.hpp
    NAMESPACE  tinyosnesc::TinyosNesc
    EXTENSIONS .nc
    OUT        tinyosnesc/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(tinyosnescCore ${_core})
target_link_libraries(tinyosnescCore PUBLIC tinyosnescGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(tinyosnesc_cli cli.cpp)
  target_include_directories(tinyosnesc_cli PRIVATE cli)
  target_link_libraries(tinyosnesc_cli PRIVATE tinyosnescCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(tinyosnescCli ${_cli})
    target_include_directories(tinyosnescCli PUBLIC cli)
    target_link_libraries(tinyosnescCli PUBLIC tinyosnescCore cursiv::cli)
    target_link_libraries(tinyosnesc_cli PRIVATE tinyosnescCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(tinyosnescLsp ${_lsp})
  target_include_directories(tinyosnescLsp PUBLIC lsp)
  target_link_libraries(tinyosnescLsp PUBLIC tinyosnescCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(tinyosnesc_lsp lsp.cpp)
    target_link_libraries(tinyosnesc_lsp PRIVATE tinyosnescLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET tinyosnescLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(tinyosnesc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(tinyosnesc)
