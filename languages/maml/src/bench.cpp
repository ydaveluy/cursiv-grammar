#include "cursiv/LanguageBench.hpp"
#include "maml/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      maml::MamlSharedServices, maml::register_lsp_services>(argc, argv);
}
