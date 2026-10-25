#include "cursiv/bench/LanguageBench.hpp"
#include "maml/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      maml::MamlSharedServices, maml::register_lsp_services>(argc, argv);
}
