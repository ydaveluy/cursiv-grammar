# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID xyz
    GRAMMAR    xyz/XyzGrammar.hpp
    NAMESPACE  xyz::Xyz
    EXTENSIONS .xyz
    OUT        xyz/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(xyzCore ${_core})
target_link_libraries(xyzCore PUBLIC xyzGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(xyz_cli cli.cpp)
  target_include_directories(xyz_cli PRIVATE cli)
  target_link_libraries(xyz_cli PRIVATE xyzCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(xyzCli ${_cli})
    target_include_directories(xyzCli PUBLIC cli)
    target_link_libraries(xyzCli PUBLIC xyzCore cursiv::cli)
    target_link_libraries(xyz_cli PRIVATE xyzCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(xyzLsp ${_lsp})
  target_include_directories(xyzLsp PUBLIC lsp)
  target_link_libraries(xyzLsp PUBLIC xyzCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(xyz_lsp lsp.cpp)
    target_link_libraries(xyz_lsp PRIVATE xyzLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET xyzLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(xyz SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(xyz)
