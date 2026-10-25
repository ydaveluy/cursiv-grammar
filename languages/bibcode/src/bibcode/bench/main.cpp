#include "cursiv/bench/LanguageBench.hpp"
#include "bibcode/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      bibcode::BibcodeSharedServices, bibcode::register_lsp_services>(argc, argv);
}
