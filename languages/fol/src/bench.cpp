#include "cursiv/LanguageBench.hpp"
#include "fol/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      fol::FolSharedServices, fol::register_lsp_services>(argc, argv);
}
