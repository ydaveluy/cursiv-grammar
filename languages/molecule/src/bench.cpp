#include "cursiv/LanguageBench.hpp"
#include "molecule/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      molecule::MoleculeSharedServices, molecule::register_lsp_services>(argc, argv);
}
