#include "cursiv/bench/LanguageBench.hpp"
#include "pdn/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pdn::PdnSharedServices, pdn::register_lsp_services>(argc, argv);
}
