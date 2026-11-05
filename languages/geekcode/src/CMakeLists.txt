# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID geekcode
    GRAMMAR    geekcode/GeekcodeGrammar.hpp
    NAMESPACE  geekcode::Geekcode
    EXTENSIONS .geek
    OUT        geekcode/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(geekcodeCore ${_core})
target_link_libraries(geekcodeCore PUBLIC geekcodeGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(geekcode_cli cli.cpp)
  target_include_directories(geekcode_cli PRIVATE cli)
  target_link_libraries(geekcode_cli PRIVATE geekcodeCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(geekcodeCli ${_cli})
    target_include_directories(geekcodeCli PUBLIC cli)
    target_link_libraries(geekcodeCli PUBLIC geekcodeCore cursiv::cli)
    target_link_libraries(geekcode_cli PRIVATE geekcodeCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(geekcodeLsp ${_lsp})
  target_include_directories(geekcodeLsp PUBLIC lsp)
  target_link_libraries(geekcodeLsp PUBLIC geekcodeCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(geekcode_lsp lsp.cpp)
    target_link_libraries(geekcode_lsp PRIVATE geekcodeLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET geekcodeLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(geekcode SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(geekcode)
