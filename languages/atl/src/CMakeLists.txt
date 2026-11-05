# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID atl
    GRAMMAR    atl/AtlGrammar.hpp
    NAMESPACE  atl::Atl
    EXTENSIONS .atl
    OUT        atl/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(atlCore ${_core})
target_link_libraries(atlCore PUBLIC atlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(atl_cli cli.cpp)
  target_include_directories(atl_cli PRIVATE cli)
  target_link_libraries(atl_cli PRIVATE atlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(atlCli ${_cli})
    target_include_directories(atlCli PUBLIC cli)
    target_link_libraries(atlCli PUBLIC atlCore cursiv::cli)
    target_link_libraries(atl_cli PRIVATE atlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(atlLsp ${_lsp})
  target_include_directories(atlLsp PUBLIC lsp)
  target_link_libraries(atlLsp PUBLIC atlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(atl_lsp lsp.cpp)
    target_link_libraries(atl_lsp PRIVATE atlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET atlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(atl SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(atl)
