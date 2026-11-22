#include "cursiv/LanguageBench.hpp"
#include "pdn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      pdn::PdnSharedServices, pdn::register_lsp_services>(argc, argv);
}
