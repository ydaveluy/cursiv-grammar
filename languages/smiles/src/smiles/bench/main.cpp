#include "cursiv/bench/LanguageBench.hpp"
#include "smiles/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      smiles::SmilesSharedServices, smiles::register_lsp_services>(argc, argv);
}
