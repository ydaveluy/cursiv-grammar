# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID objc
    GRAMMAR    objc/ObjcGrammar.hpp
    NAMESPACE  objc::Objc
    EXTENSIONS .m
    OUT        objc/generated)

# The AST bindings exceed GCC's default constexpr evaluation budget.
target_compile_options(objcGenerated PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-fconstexpr-ops-limit=2147483647>)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(objcCore ${_core})
target_link_libraries(objcCore PUBLIC objcGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(objc_cli cli.cpp)
  target_include_directories(objc_cli PRIVATE cli)
  target_link_libraries(objc_cli PRIVATE objcCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(objcCli ${_cli})
    target_include_directories(objcCli PUBLIC cli)
    target_link_libraries(objcCli PUBLIC objcCore cursiv::cli)
    target_link_libraries(objc_cli PRIVATE objcCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(objcLsp ${_lsp})
  target_include_directories(objcLsp PUBLIC lsp)
  target_link_libraries(objcLsp PUBLIC objcCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(objc_lsp lsp.cpp)
    target_link_libraries(objc_lsp PRIVATE objcLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET objcLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(objc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(objc)
