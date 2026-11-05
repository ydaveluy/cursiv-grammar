#include "cursiv/LanguageBench.hpp"
#include "terraform/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      terraform::TerraformSharedServices, terraform::register_lsp_services>(argc, argv);
}
