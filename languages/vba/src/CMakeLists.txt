# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID vba
    GRAMMAR    vba/VbaGrammar.hpp
    NAMESPACE  vba::Vba
    EXTENSIONS .bas .cls .frm
    OUT        vba/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(vbaCore ${_core})
target_link_libraries(vbaCore PUBLIC vbaGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(vba_cli cli.cpp)
  target_include_directories(vba_cli PRIVATE cli)
  target_link_libraries(vba_cli PRIVATE vbaCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(vbaCli ${_cli})
    target_include_directories(vbaCli PUBLIC cli)
    target_link_libraries(vbaCli PUBLIC vbaCore cursiv::cli)
    target_link_libraries(vba_cli PRIVATE vbaCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(vbaLsp ${_lsp})
  target_include_directories(vbaLsp PUBLIC lsp)
  target_link_libraries(vbaLsp PUBLIC vbaCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(vba_lsp lsp.cpp)
    target_link_libraries(vba_lsp PRIVATE vbaLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET vbaLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(vba SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(vba)
