# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID tinybasic
    GRAMMAR    tinybasic/TinybasicGrammar.hpp
    NAMESPACE  tinybasic::Tinybasic
    EXTENSIONS .tinybasic
    OUT        tinybasic/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(tinybasicCore ${_core})
target_link_libraries(tinybasicCore PUBLIC tinybasicGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(tinybasic_cli cli.cpp)
  target_include_directories(tinybasic_cli PRIVATE cli)
  target_link_libraries(tinybasic_cli PRIVATE tinybasicCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(tinybasicCli ${_cli})
    target_include_directories(tinybasicCli PUBLIC cli)
    target_link_libraries(tinybasicCli PUBLIC tinybasicCore cursiv::cli)
    target_link_libraries(tinybasic_cli PRIVATE tinybasicCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(tinybasicLsp ${_lsp})
  target_include_directories(tinybasicLsp PUBLIC lsp)
  target_link_libraries(tinybasicLsp PUBLIC tinybasicCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(tinybasic_lsp lsp.cpp)
    target_link_libraries(tinybasic_lsp PRIVATE tinybasicLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET tinybasicLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(tinybasic SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(tinybasic)
