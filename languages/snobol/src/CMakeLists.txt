# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID snobol
    GRAMMAR    snobol/SnobolGrammar.hpp
    NAMESPACE  snobol::Snobol
    EXTENSIONS .sno
    OUT        snobol/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(snobolCore ${_core})
target_link_libraries(snobolCore PUBLIC snobolGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(snobol_cli cli.cpp)
  target_include_directories(snobol_cli PRIVATE cli)
  target_link_libraries(snobol_cli PRIVATE snobolCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(snobolCli ${_cli})
    target_include_directories(snobolCli PUBLIC cli)
    target_link_libraries(snobolCli PUBLIC snobolCore cursiv::cli)
    target_link_libraries(snobol_cli PRIVATE snobolCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(snobolLsp ${_lsp})
  target_include_directories(snobolLsp PUBLIC lsp)
  target_link_libraries(snobolLsp PUBLIC snobolCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(snobol_lsp lsp.cpp)
    target_link_libraries(snobol_lsp PRIVATE snobolLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET snobolLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(snobol SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(snobol)
