#include "cursiv/bench/LanguageBench.hpp"
#include "bibtex/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      bibtex::BibtexSharedServices, bibtex::register_lsp_services>(argc, argv);
}
