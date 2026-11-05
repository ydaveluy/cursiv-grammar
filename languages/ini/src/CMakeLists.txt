# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID         ini
    GRAMMAR    ini/IniGrammar.hpp
    NAMESPACE  ini::INI
    EXTENSIONS .ini
    OUT        ini/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(iniCore ${_core})
target_link_libraries(iniCore PUBLIC iniGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(ini_cli cli.cpp)
  target_include_directories(ini_cli PRIVATE cli)
  target_link_libraries(ini_cli PRIVATE iniCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(iniCli ${_cli})
    target_include_directories(iniCli PUBLIC cli)
    target_link_libraries(iniCli PUBLIC iniCore cursiv::cli)
    target_link_libraries(ini_cli PRIVATE iniCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(iniLsp ${_lsp})
  target_include_directories(iniLsp PUBLIC lsp)
  target_link_libraries(iniLsp PUBLIC iniCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(ini_lsp lsp.cpp)
    target_link_libraries(ini_lsp PRIVATE iniLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET iniLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(ini SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(ini)
