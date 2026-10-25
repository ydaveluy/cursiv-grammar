#include "cursiv/bench/LanguageBench.hpp"
#include "fol/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      fol::FolSharedServices, fol::register_lsp_services>(argc, argv);
}
