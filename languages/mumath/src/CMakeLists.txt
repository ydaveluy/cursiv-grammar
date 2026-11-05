# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID mumath
    GRAMMAR    mumath/MumathGrammar.hpp
    NAMESPACE  mumath::Mumath
    EXTENSIONS .mu
    OUT        mumath/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(mumathCore ${_core})
target_link_libraries(mumathCore PUBLIC mumathGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(mumath_cli cli.cpp)
  target_include_directories(mumath_cli PRIVATE cli)
  target_link_libraries(mumath_cli PRIVATE mumathCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(mumathCli ${_cli})
    target_include_directories(mumathCli PUBLIC cli)
    target_link_libraries(mumathCli PUBLIC mumathCore cursiv::cli)
    target_link_libraries(mumath_cli PRIVATE mumathCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(mumathLsp ${_lsp})
  target_include_directories(mumathLsp PUBLIC lsp)
  target_link_libraries(mumathLsp PUBLIC mumathCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(mumath_lsp lsp.cpp)
    target_link_libraries(mumath_lsp PRIVATE mumathLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET mumathLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(mumath SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(mumath)
