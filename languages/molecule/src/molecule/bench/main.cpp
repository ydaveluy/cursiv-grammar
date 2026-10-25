#include "cursiv/bench/LanguageBench.hpp"
#include "molecule/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      molecule::MoleculeSharedServices, molecule::register_lsp_services>(argc, argv);
}
