#include "cursiv/bench/LanguageBench.hpp"
#include "terraform/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      terraform::TerraformSharedServices, terraform::register_lsp_services>(argc, argv);
}
