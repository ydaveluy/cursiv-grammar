# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID terraform
    GRAMMAR    terraform/TerraformGrammar.hpp
    NAMESPACE  terraform::Terraform
    EXTENSIONS .tf
    OUT        terraform/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(terraformCore ${_core})
target_link_libraries(terraformCore PUBLIC terraformGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(terraform_cli cli.cpp)
  target_include_directories(terraform_cli PRIVATE cli)
  target_link_libraries(terraform_cli PRIVATE terraformCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(terraformCli ${_cli})
    target_include_directories(terraformCli PUBLIC cli)
    target_link_libraries(terraformCli PUBLIC terraformCore cursiv::cli)
    target_link_libraries(terraform_cli PRIVATE terraformCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(terraformLsp ${_lsp})
  target_include_directories(terraformLsp PUBLIC lsp)
  target_link_libraries(terraformLsp PUBLIC terraformCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(terraform_lsp lsp.cpp)
    target_link_libraries(terraform_lsp PRIVATE terraformLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET terraformLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(terraform SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(terraform)
