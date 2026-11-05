#include "cursiv/LanguageBench.hpp"
#include "bibcode/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      bibcode::BibcodeSharedServices, bibcode::register_lsp_services>(argc, argv);
}
