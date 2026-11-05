# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID bdf
    GRAMMAR    bdf/BdfGrammar.hpp
    NAMESPACE  bdf::Bdf
    EXTENSIONS .bdf
    OUT        bdf/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(bdfCore ${_core})
target_link_libraries(bdfCore PUBLIC bdfGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(bdf_cli cli.cpp)
  target_include_directories(bdf_cli PRIVATE cli)
  target_link_libraries(bdf_cli PRIVATE bdfCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(bdfCli ${_cli})
    target_include_directories(bdfCli PUBLIC cli)
    target_link_libraries(bdfCli PUBLIC bdfCore cursiv::cli)
    target_link_libraries(bdf_cli PRIVATE bdfCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(bdfLsp ${_lsp})
  target_include_directories(bdfLsp PUBLIC lsp)
  target_link_libraries(bdfLsp PUBLIC bdfCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(bdf_lsp lsp.cpp)
    target_link_libraries(bdf_lsp PRIVATE bdfLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET bdfLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(bdf SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(bdf)
