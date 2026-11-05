# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID metamath
    GRAMMAR    metamath/MetamathGrammar.hpp
    NAMESPACE  metamath::Metamath
    EXTENSIONS .mm
    OUT        metamath/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(metamathCore ${_core})
target_link_libraries(metamathCore PUBLIC metamathGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(metamath_cli cli.cpp)
  target_include_directories(metamath_cli PRIVATE cli)
  target_link_libraries(metamath_cli PRIVATE metamathCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(metamathCli ${_cli})
    target_include_directories(metamathCli PUBLIC cli)
    target_link_libraries(metamathCli PUBLIC metamathCore cursiv::cli)
    target_link_libraries(metamath_cli PRIVATE metamathCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(metamathLsp ${_lsp})
  target_include_directories(metamathLsp PUBLIC lsp)
  target_link_libraries(metamathLsp PUBLIC metamathCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(metamath_lsp lsp.cpp)
    target_link_libraries(metamath_lsp PRIVATE metamathLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET metamathLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(metamath SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(metamath)
