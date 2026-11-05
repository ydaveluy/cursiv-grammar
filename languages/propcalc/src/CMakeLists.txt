# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID propcalc
    GRAMMAR    propcalc/PropcalcGrammar.hpp
    NAMESPACE  propcalc::Propcalc
    EXTENSIONS .propcalc
    OUT        propcalc/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(propcalcCore ${_core})
target_link_libraries(propcalcCore PUBLIC propcalcGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(propcalc_cli cli.cpp)
  target_include_directories(propcalc_cli PRIVATE cli)
  target_link_libraries(propcalc_cli PRIVATE propcalcCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(propcalcCli ${_cli})
    target_include_directories(propcalcCli PUBLIC cli)
    target_link_libraries(propcalcCli PUBLIC propcalcCore cursiv::cli)
    target_link_libraries(propcalc_cli PRIVATE propcalcCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(propcalcLsp ${_lsp})
  target_include_directories(propcalcLsp PUBLIC lsp)
  target_link_libraries(propcalcLsp PUBLIC propcalcCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(propcalc_lsp lsp.cpp)
    target_link_libraries(propcalc_lsp PRIVATE propcalcLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET propcalcLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(propcalc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(propcalc)
