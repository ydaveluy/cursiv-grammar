# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID szf
    GRAMMAR    szf/SzfGrammar.hpp
    NAMESPACE  szf::Szf
    EXTENSIONS .sfz .szf
    OUT        szf/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(szfCore ${_core})
target_link_libraries(szfCore PUBLIC szfGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(szf_cli cli.cpp)
  target_include_directories(szf_cli PRIVATE cli)
  target_link_libraries(szf_cli PRIVATE szfCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(szfCli ${_cli})
    target_include_directories(szfCli PUBLIC cli)
    target_link_libraries(szfCli PUBLIC szfCore cursiv::cli)
    target_link_libraries(szf_cli PRIVATE szfCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(szfLsp ${_lsp})
  target_include_directories(szfLsp PUBLIC lsp)
  target_link_libraries(szfLsp PUBLIC szfCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(szf_lsp lsp.cpp)
    target_link_libraries(szf_lsp PRIVATE szfLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET szfLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(szf SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(szf)
