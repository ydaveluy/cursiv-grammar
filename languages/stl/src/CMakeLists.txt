# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID stl
    GRAMMAR    stl/StlGrammar.hpp
    NAMESPACE  stl::Stl
    EXTENSIONS .stl
    OUT        stl/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(stlCore ${_core})
target_link_libraries(stlCore PUBLIC stlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(stl_cli cli.cpp)
  target_include_directories(stl_cli PRIVATE cli)
  target_link_libraries(stl_cli PRIVATE stlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(stlCli ${_cli})
    target_include_directories(stlCli PUBLIC cli)
    target_link_libraries(stlCli PUBLIC stlCore cursiv::cli)
    target_link_libraries(stl_cli PRIVATE stlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(stlLsp ${_lsp})
  target_include_directories(stlLsp PUBLIC lsp)
  target_link_libraries(stlLsp PUBLIC stlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(stl_lsp lsp.cpp)
    target_link_libraries(stl_lsp PRIVATE stlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET stlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(stl SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(stl)
