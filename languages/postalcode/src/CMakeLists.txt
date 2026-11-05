# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID postalcode
    GRAMMAR    postalcode/PostalcodeGrammar.hpp
    NAMESPACE  postalcode::Postalcode
    EXTENSIONS .postalcode
    OUT        postalcode/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(postalcodeCore ${_core})
target_link_libraries(postalcodeCore PUBLIC postalcodeGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(postalcode_cli cli.cpp)
  target_include_directories(postalcode_cli PRIVATE cli)
  target_link_libraries(postalcode_cli PRIVATE postalcodeCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(postalcodeCli ${_cli})
    target_include_directories(postalcodeCli PUBLIC cli)
    target_link_libraries(postalcodeCli PUBLIC postalcodeCore cursiv::cli)
    target_link_libraries(postalcode_cli PRIVATE postalcodeCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(postalcodeLsp ${_lsp})
  target_include_directories(postalcodeLsp PUBLIC lsp)
  target_link_libraries(postalcodeLsp PUBLIC postalcodeCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(postalcode_lsp lsp.cpp)
    target_link_libraries(postalcode_lsp PRIVATE postalcodeLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET postalcodeLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(postalcode SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(postalcode)
