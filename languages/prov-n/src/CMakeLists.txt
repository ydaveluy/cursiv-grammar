# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID provn
    GRAMMAR    provn/ProvNGrammar.hpp
    NAMESPACE  provn::ProvN
    EXTENSIONS .provn
    OUT        provn/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(provnCore ${_core})
target_link_libraries(provnCore PUBLIC provnGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(provn_cli cli.cpp)
  target_include_directories(provn_cli PRIVATE cli)
  target_link_libraries(provn_cli PRIVATE provnCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(provnCli ${_cli})
    target_include_directories(provnCli PUBLIC cli)
    target_link_libraries(provnCli PUBLIC provnCore cursiv::cli)
    target_link_libraries(provn_cli PRIVATE provnCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(provnLsp ${_lsp})
  target_include_directories(provnLsp PUBLIC lsp)
  target_link_libraries(provnLsp PUBLIC provnCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(provn_lsp lsp.cpp)
    target_link_libraries(provn_lsp PRIVATE provnLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET provnLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(provn SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(provn)
