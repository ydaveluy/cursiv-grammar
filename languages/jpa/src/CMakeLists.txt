# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID jpa
    GRAMMAR    jpa/JpaGrammar.hpp
    NAMESPACE  jpa::Jpa
    EXTENSIONS .jpql
    OUT        jpa/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(jpaCore ${_core})
target_link_libraries(jpaCore PUBLIC jpaGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(jpa_cli cli.cpp)
  target_include_directories(jpa_cli PRIVATE cli)
  target_link_libraries(jpa_cli PRIVATE jpaCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(jpaCli ${_cli})
    target_include_directories(jpaCli PUBLIC cli)
    target_link_libraries(jpaCli PUBLIC jpaCore cursiv::cli)
    target_link_libraries(jpa_cli PRIVATE jpaCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(jpaLsp ${_lsp})
  target_include_directories(jpaLsp PUBLIC lsp)
  target_link_libraries(jpaLsp PUBLIC jpaCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(jpa_lsp lsp.cpp)
    target_link_libraries(jpa_lsp PRIVATE jpaLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET jpaLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(jpa SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(jpa)
