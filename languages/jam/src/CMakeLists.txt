# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID jam
    GRAMMAR    jam/JamGrammar.hpp
    NAMESPACE  jam::Jam
    EXTENSIONS .jam
    OUT        jam/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(jamCore ${_core})
target_link_libraries(jamCore PUBLIC jamGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(jam_cli cli.cpp)
  target_include_directories(jam_cli PRIVATE cli)
  target_link_libraries(jam_cli PRIVATE jamCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(jamCli ${_cli})
    target_include_directories(jamCli PUBLIC cli)
    target_link_libraries(jamCli PUBLIC jamCore cursiv::cli)
    target_link_libraries(jam_cli PRIVATE jamCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(jamLsp ${_lsp})
  target_include_directories(jamLsp PUBLIC lsp)
  target_link_libraries(jamLsp PUBLIC jamCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(jam_lsp lsp.cpp)
    target_link_libraries(jam_lsp PRIVATE jamLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET jamLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(jam SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(jam)
