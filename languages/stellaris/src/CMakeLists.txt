# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID stellaris
    GRAMMAR    stellaris/StellarisGrammar.hpp
    NAMESPACE  stellaris::Stellaris
    EXTENSIONS .stellaris
    OUT        stellaris/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(stellarisCore ${_core})
target_link_libraries(stellarisCore PUBLIC stellarisGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(stellaris_cli cli.cpp)
  target_include_directories(stellaris_cli PRIVATE cli)
  target_link_libraries(stellaris_cli PRIVATE stellarisCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(stellarisCli ${_cli})
    target_include_directories(stellarisCli PUBLIC cli)
    target_link_libraries(stellarisCli PUBLIC stellarisCore cursiv::cli)
    target_link_libraries(stellaris_cli PRIVATE stellarisCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(stellarisLsp ${_lsp})
  target_include_directories(stellarisLsp PUBLIC lsp)
  target_link_libraries(stellarisLsp PUBLIC stellarisCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(stellaris_lsp lsp.cpp)
    target_link_libraries(stellaris_lsp PRIVATE stellarisLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET stellarisLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(stellaris SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(stellaris)
