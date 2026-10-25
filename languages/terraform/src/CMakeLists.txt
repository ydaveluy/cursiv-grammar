# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `terraformGenerated` (the compiled library) and `terraform_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID terraform
    GRAMMAR    terraform/core/TerraformGrammar.hpp
    NAMESPACE  terraform::Terraform
    EXTENSIONS .tf
    OUT        terraform/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/terraform/core/*.cpp)
add_library(terraformCore ${_core})
target_link_libraries(terraformCore PUBLIC terraformGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/terraform/cli/*.cpp)
add_executable(terraform_cli ${_cli})
target_link_libraries(terraform_cli
                      PRIVATE terraformCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/terraform/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/terraform/lsp/main.cpp)
add_library(terraformLsp ${_lsp})
target_link_libraries(terraformLsp PUBLIC terraformCore cursiv::lsp)
add_executable(terraform_lsp terraform/lsp/main.cpp)
target_link_libraries(terraform_lsp PRIVATE terraformLsp)

cursiv_add_benchmark(terraform SOURCES terraform/bench/main.cpp)

cursiv_package_language(terraform)
