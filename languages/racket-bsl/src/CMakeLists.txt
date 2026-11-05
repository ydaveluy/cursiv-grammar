# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID racketbsl
    GRAMMAR    racketbsl/RacketBslGrammar.hpp
    NAMESPACE  racketbsl::RacketBsl
    EXTENSIONS .rkt
    OUT        racketbsl/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(racketbslCore ${_core})
target_link_libraries(racketbslCore PUBLIC racketbslGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(racketbsl_cli cli.cpp)
  target_include_directories(racketbsl_cli PRIVATE cli)
  target_link_libraries(racketbsl_cli PRIVATE racketbslCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(racketbslCli ${_cli})
    target_include_directories(racketbslCli PUBLIC cli)
    target_link_libraries(racketbslCli PUBLIC racketbslCore cursiv::cli)
    target_link_libraries(racketbsl_cli PRIVATE racketbslCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(racketbslLsp ${_lsp})
  target_include_directories(racketbslLsp PUBLIC lsp)
  target_link_libraries(racketbslLsp PUBLIC racketbslCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(racketbsl_lsp lsp.cpp)
    target_link_libraries(racketbsl_lsp PRIVATE racketbslLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET racketbslLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(racketbsl SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(racketbsl)
