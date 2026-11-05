# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID rexx
    GRAMMAR    rexx/RexxGrammar.hpp
    NAMESPACE  rexx::Rexx
    EXTENSIONS .rex
    OUT        rexx/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(rexxCore ${_core})
target_link_libraries(rexxCore PUBLIC rexxGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(rexx_cli cli.cpp)
  target_include_directories(rexx_cli PRIVATE cli)
  target_link_libraries(rexx_cli PRIVATE rexxCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(rexxCli ${_cli})
    target_include_directories(rexxCli PUBLIC cli)
    target_link_libraries(rexxCli PUBLIC rexxCore cursiv::cli)
    target_link_libraries(rexx_cli PRIVATE rexxCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(rexxLsp ${_lsp})
  target_include_directories(rexxLsp PUBLIC lsp)
  target_link_libraries(rexxLsp PUBLIC rexxCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(rexx_lsp lsp.cpp)
    target_link_libraries(rexx_lsp PRIVATE rexxLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET rexxLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(rexx SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(rexx)
