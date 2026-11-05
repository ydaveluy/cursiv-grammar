# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID dcm
    GRAMMAR    dcm/DcmGrammar.hpp
    NAMESPACE  dcm::Dcm
    EXTENSIONS .dcm
    OUT        dcm/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(dcmCore ${_core})
target_link_libraries(dcmCore PUBLIC dcmGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(dcm_cli cli.cpp)
  target_include_directories(dcm_cli PRIVATE cli)
  target_link_libraries(dcm_cli PRIVATE dcmCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(dcmCli ${_cli})
    target_include_directories(dcmCli PUBLIC cli)
    target_link_libraries(dcmCli PUBLIC dcmCore cursiv::cli)
    target_link_libraries(dcm_cli PRIVATE dcmCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(dcmLsp ${_lsp})
  target_include_directories(dcmLsp PUBLIC lsp)
  target_link_libraries(dcmLsp PUBLIC dcmCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(dcm_lsp lsp.cpp)
    target_link_libraries(dcm_lsp PRIVATE dcmLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET dcmLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(dcm SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(dcm)
