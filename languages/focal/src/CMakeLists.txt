# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID focal
    GRAMMAR    focal/FocalGrammar.hpp
    NAMESPACE  focal::Focal
    EXTENSIONS .focal
    OUT        focal/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(focalCore ${_core})
target_link_libraries(focalCore PUBLIC focalGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(focal_cli cli.cpp)
  target_include_directories(focal_cli PRIVATE cli)
  target_link_libraries(focal_cli PRIVATE focalCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(focalCli ${_cli})
    target_include_directories(focalCli PUBLIC cli)
    target_link_libraries(focalCli PUBLIC focalCore cursiv::cli)
    target_link_libraries(focal_cli PRIVATE focalCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(focalLsp ${_lsp})
  target_include_directories(focalLsp PUBLIC lsp)
  target_link_libraries(focalLsp PUBLIC focalCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(focal_lsp lsp.cpp)
    target_link_libraries(focal_lsp PRIVATE focalLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET focalLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(focal SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(focal)
