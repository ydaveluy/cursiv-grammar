#include "cursiv/LanguageBench.hpp"
#include "molecule/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      molecule::MoleculeSharedServices, molecule::register_lsp_services>(argc, argv);
}
