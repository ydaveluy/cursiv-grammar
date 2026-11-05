# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID ocl
    GRAMMAR    ocl/OclGrammar.hpp
    NAMESPACE  ocl::Ocl
    EXTENSIONS .ocl
    OUT        ocl/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(oclCore ${_core})
target_link_libraries(oclCore PUBLIC oclGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(ocl_cli cli.cpp)
  target_include_directories(ocl_cli PRIVATE cli)
  target_link_libraries(ocl_cli PRIVATE oclCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(oclCli ${_cli})
    target_include_directories(oclCli PUBLIC cli)
    target_link_libraries(oclCli PUBLIC oclCore cursiv::cli)
    target_link_libraries(ocl_cli PRIVATE oclCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(oclLsp ${_lsp})
  target_include_directories(oclLsp PUBLIC lsp)
  target_link_libraries(oclLsp PUBLIC oclCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(ocl_lsp lsp.cpp)
    target_link_libraries(ocl_lsp PRIVATE oclLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET oclLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(ocl SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(ocl)
