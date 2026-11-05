# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID mps
    GRAMMAR    mps/MpsGrammar.hpp
    NAMESPACE  mps::Mps
    EXTENSIONS .mps
    OUT        mps/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(mpsCore ${_core})
target_link_libraries(mpsCore PUBLIC mpsGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(mps_cli cli.cpp)
  target_include_directories(mps_cli PRIVATE cli)
  target_link_libraries(mps_cli PRIVATE mpsCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(mpsCli ${_cli})
    target_include_directories(mpsCli PUBLIC cli)
    target_link_libraries(mpsCli PUBLIC mpsCore cursiv::cli)
    target_link_libraries(mps_cli PRIVATE mpsCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(mpsLsp ${_lsp})
  target_include_directories(mpsLsp PUBLIC lsp)
  target_link_libraries(mpsLsp PUBLIC mpsCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(mps_lsp lsp.cpp)
    target_link_libraries(mps_lsp PRIVATE mpsLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET mpsLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(mps SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(mps)
