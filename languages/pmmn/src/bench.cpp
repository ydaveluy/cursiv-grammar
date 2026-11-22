#include "cursiv/LanguageBench.hpp"
#include "pmmn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      pmmn::PmmnSharedServices, pmmn::register_lsp_services>(argc, argv);
}
