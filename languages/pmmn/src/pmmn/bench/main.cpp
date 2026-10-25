#include "cursiv/bench/LanguageBench.hpp"
#include "pmmn/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pmmn::PmmnSharedServices, pmmn::register_lsp_services>(argc, argv);
}
