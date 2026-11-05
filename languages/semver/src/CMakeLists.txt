# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID semver
    GRAMMAR    semver/SemverGrammar.hpp
    NAMESPACE  semver::Semver
    EXTENSIONS .semver
    OUT        semver/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(semverCore ${_core})
target_link_libraries(semverCore PUBLIC semverGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(semver_cli cli.cpp)
  target_include_directories(semver_cli PRIVATE cli)
  target_link_libraries(semver_cli PRIVATE semverCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(semverCli ${_cli})
    target_include_directories(semverCli PUBLIC cli)
    target_link_libraries(semverCli PUBLIC semverCore cursiv::cli)
    target_link_libraries(semver_cli PRIVATE semverCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(semverLsp ${_lsp})
  target_include_directories(semverLsp PUBLIC lsp)
  target_link_libraries(semverLsp PUBLIC semverCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(semver_lsp lsp.cpp)
    target_link_libraries(semver_lsp PRIVATE semverLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET semverLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(semver SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(semver)
