#include "cursiv/LanguageBench.hpp"
#include "metamath/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      metamath::MetamathSharedServices, metamath::register_lsp_services>(argc, argv);
}
