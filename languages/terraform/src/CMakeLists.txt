# Generate the parser and its language module.
cursiv_generate_language(ID terraform
    GRAMMAR    terraform/core/TerraformGrammar.hpp
    NAMESPACE  terraform::Terraform
    EXTENSIONS .tf
    OUT        terraform/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/terraform/core/*.cpp)
add_library(terraformCore ${_core})
target_link_libraries(terraformCore PUBLIC terraformGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/terraform/cli/*.cpp)
if(_cli)
  add_executable(terraform_cli ${_cli})
  target_link_libraries(terraform_cli
                        PRIVATE terraformCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/terraform/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/terraform/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/terraform/lsp/main.cpp)
  add_library(terraformLsp ${_lsp})
  target_link_libraries(terraformLsp PUBLIC terraformCore cursiv::lsp)
  add_executable(terraform_lsp terraform/lsp/main.cpp)
  target_link_libraries(terraform_lsp PRIVATE terraformLsp)
endif()

if(TARGET terraformLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/terraform/bench/main.cpp")
  cursiv_add_benchmark(terraform
      SOURCES terraform/bench/main.cpp)
endif()

cursiv_package_language(terraform)
