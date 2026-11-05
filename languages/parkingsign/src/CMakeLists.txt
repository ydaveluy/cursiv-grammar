# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID parkingsign
    GRAMMAR    parkingsign/ParkingsignGrammar.hpp
    NAMESPACE  parkingsign::Parkingsign
    EXTENSIONS .parkingsign
    OUT        parkingsign/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(parkingsignCore ${_core})
target_link_libraries(parkingsignCore PUBLIC parkingsignGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(parkingsign_cli cli.cpp)
  target_include_directories(parkingsign_cli PRIVATE cli)
  target_link_libraries(parkingsign_cli PRIVATE parkingsignCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(parkingsignCli ${_cli})
    target_include_directories(parkingsignCli PUBLIC cli)
    target_link_libraries(parkingsignCli PUBLIC parkingsignCore cursiv::cli)
    target_link_libraries(parkingsign_cli PRIVATE parkingsignCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(parkingsignLsp ${_lsp})
  target_include_directories(parkingsignLsp PUBLIC lsp)
  target_link_libraries(parkingsignLsp PUBLIC parkingsignCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(parkingsign_lsp lsp.cpp)
    target_link_libraries(parkingsign_lsp PRIVATE parkingsignLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET parkingsignLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(parkingsign SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(parkingsign)
