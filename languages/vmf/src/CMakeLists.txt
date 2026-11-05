# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID vmf
    GRAMMAR    vmf/VmfGrammar.hpp
    NAMESPACE  vmf::Vmf
    EXTENSIONS .vmf
    OUT        vmf/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(vmfCore ${_core})
target_link_libraries(vmfCore PUBLIC vmfGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(vmf_cli cli.cpp)
  target_include_directories(vmf_cli PRIVATE cli)
  target_link_libraries(vmf_cli PRIVATE vmfCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(vmfCli ${_cli})
    target_include_directories(vmfCli PUBLIC cli)
    target_link_libraries(vmfCli PUBLIC vmfCore cursiv::cli)
    target_link_libraries(vmf_cli PRIVATE vmfCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(vmfLsp ${_lsp})
  target_include_directories(vmfLsp PUBLIC lsp)
  target_link_libraries(vmfLsp PUBLIC vmfCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(vmf_lsp lsp.cpp)
    target_link_libraries(vmf_lsp PRIVATE vmfLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET vmfLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(vmf SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(vmf)
