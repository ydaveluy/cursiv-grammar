#include "cursiv/bench/LanguageBench.hpp"
#include "metamath/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      metamath::MetamathSharedServices, metamath::register_lsp_services>(argc, argv);
}
