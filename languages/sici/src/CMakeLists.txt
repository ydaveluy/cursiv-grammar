# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID sici
    GRAMMAR    sici/SiciGrammar.hpp
    NAMESPACE  sici::Sici
    EXTENSIONS .sici
    OUT        sici/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(siciCore ${_core})
target_link_libraries(siciCore PUBLIC siciGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(sici_cli cli.cpp)
  target_include_directories(sici_cli PRIVATE cli)
  target_link_libraries(sici_cli PRIVATE siciCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(siciCli ${_cli})
    target_include_directories(siciCli PUBLIC cli)
    target_link_libraries(siciCli PUBLIC siciCore cursiv::cli)
    target_link_libraries(sici_cli PRIVATE siciCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(siciLsp ${_lsp})
  target_include_directories(siciLsp PUBLIC lsp)
  target_link_libraries(siciLsp PUBLIC siciCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(sici_lsp lsp.cpp)
    target_link_libraries(sici_lsp PRIVATE siciLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET siciLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(sici SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(sici)
