# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID ruleworks
    GRAMMAR    ruleworks/RuleworksGrammar.hpp
    NAMESPACE  ruleworks::Ruleworks
    EXTENSIONS .rul
    OUT        ruleworks/generated)

# The grammar's write-action table is large enough (~150 terminals, ~150
# rules) to need more than the compiler's default template recursion depth.
if(CMAKE_CROSSCOMPILING)
  cursiv_host_manifest("target_compile_options(ruleworks_gen PRIVATE -ftemplate-depth=4000)")
else()
  target_compile_options(ruleworks_gen PRIVATE -ftemplate-depth=4000)
endif()
target_compile_options(ruleworksGenerated PRIVATE -ftemplate-depth=4000)
# The generated reflection and grammar metadata also exceed GCC's default
# constexpr budget; match the budget used by Cursiv's grammar generators.
target_compile_options(ruleworksGenerated PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-fconstexpr-ops-limit=2147483647>)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(ruleworksCore ${_core})
target_link_libraries(ruleworksCore PUBLIC ruleworksGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(ruleworks_cli cli.cpp)
  target_include_directories(ruleworks_cli PRIVATE cli)
  target_link_libraries(ruleworks_cli PRIVATE ruleworksCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(ruleworksCli ${_cli})
    target_include_directories(ruleworksCli PUBLIC cli)
    target_link_libraries(ruleworksCli PUBLIC ruleworksCore cursiv::cli)
    target_link_libraries(ruleworks_cli PRIVATE ruleworksCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(ruleworksLsp ${_lsp})
  target_include_directories(ruleworksLsp PUBLIC lsp)
  target_link_libraries(ruleworksLsp PUBLIC ruleworksCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(ruleworks_lsp lsp.cpp)
    target_link_libraries(ruleworks_lsp PRIVATE ruleworksLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET ruleworksLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(ruleworks SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(ruleworks)
