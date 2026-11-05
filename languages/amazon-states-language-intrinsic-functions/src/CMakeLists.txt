# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID amazonstateslanguageintrinsicfunctions
    GRAMMAR    amazonstateslanguageintrinsicfunctions/AmazonStatesLanguageIntrinsicFunctionsGrammar.hpp
    NAMESPACE  amazonstateslanguageintrinsicfunctions::AmazonStatesLanguageIntrinsicFunctions
    EXTENSIONS .asl-intrinsic
    OUT        amazonstateslanguageintrinsicfunctions/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(amazonstateslanguageintrinsicfunctionsCore ${_core})
target_link_libraries(amazonstateslanguageintrinsicfunctionsCore PUBLIC amazonstateslanguageintrinsicfunctionsGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(amazonstateslanguageintrinsicfunctions_cli cli.cpp)
  target_include_directories(amazonstateslanguageintrinsicfunctions_cli PRIVATE cli)
  target_link_libraries(amazonstateslanguageintrinsicfunctions_cli PRIVATE amazonstateslanguageintrinsicfunctionsCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(amazonstateslanguageintrinsicfunctionsCli ${_cli})
    target_include_directories(amazonstateslanguageintrinsicfunctionsCli PUBLIC cli)
    target_link_libraries(amazonstateslanguageintrinsicfunctionsCli PUBLIC amazonstateslanguageintrinsicfunctionsCore cursiv::cli)
    target_link_libraries(amazonstateslanguageintrinsicfunctions_cli PRIVATE amazonstateslanguageintrinsicfunctionsCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(amazonstateslanguageintrinsicfunctionsLsp ${_lsp})
  target_include_directories(amazonstateslanguageintrinsicfunctionsLsp PUBLIC lsp)
  target_link_libraries(amazonstateslanguageintrinsicfunctionsLsp PUBLIC amazonstateslanguageintrinsicfunctionsCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(amazonstateslanguageintrinsicfunctions_lsp lsp.cpp)
    target_link_libraries(amazonstateslanguageintrinsicfunctions_lsp PRIVATE amazonstateslanguageintrinsicfunctionsLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET amazonstateslanguageintrinsicfunctionsLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(amazonstateslanguageintrinsicfunctions SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(amazonstateslanguageintrinsicfunctions)
