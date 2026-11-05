# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID amazonstateslanguage
    GRAMMAR    amazonstateslanguage/AmazonStatesLanguageGrammar.hpp
    NAMESPACE  amazonstateslanguage::AmazonStatesLanguage
    EXTENSIONS .asl
    OUT        amazonstateslanguage/generated)

# The field-name vocabulary (~120 keyword terminals sharing a handful of
# large alternations) needs more than the compiler's default template
# recursion depth.
if(CMAKE_CROSSCOMPILING)
  cursiv_host_manifest("target_compile_options(amazonstateslanguage_gen PRIVATE -ftemplate-depth=4000)")
else()
  target_compile_options(amazonstateslanguage_gen PRIVATE -ftemplate-depth=4000)
endif()
target_compile_options(amazonstateslanguageGenerated PRIVATE -ftemplate-depth=4000)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(amazonstateslanguageCore ${_core})
target_link_libraries(amazonstateslanguageCore PUBLIC amazonstateslanguageGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(amazonstateslanguage_cli cli.cpp)
  target_include_directories(amazonstateslanguage_cli PRIVATE cli)
  target_link_libraries(amazonstateslanguage_cli PRIVATE amazonstateslanguageCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(amazonstateslanguageCli ${_cli})
    target_include_directories(amazonstateslanguageCli PUBLIC cli)
    target_link_libraries(amazonstateslanguageCli PUBLIC amazonstateslanguageCore cursiv::cli)
    target_link_libraries(amazonstateslanguage_cli PRIVATE amazonstateslanguageCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(amazonstateslanguageLsp ${_lsp})
  target_include_directories(amazonstateslanguageLsp PUBLIC lsp)
  target_link_libraries(amazonstateslanguageLsp PUBLIC amazonstateslanguageCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(amazonstateslanguage_lsp lsp.cpp)
    target_link_libraries(amazonstateslanguage_lsp PRIVATE amazonstateslanguageLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET amazonstateslanguageLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(amazonstateslanguage SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(amazonstateslanguage)
