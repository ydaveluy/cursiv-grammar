#include "cursiv/LanguageBench.hpp"
#include "smiles/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      smiles::SmilesSharedServices, smiles::register_lsp_services>(argc, argv);
}
