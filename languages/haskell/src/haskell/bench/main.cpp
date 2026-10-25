#include "cursiv/bench/LanguageBench.hpp"
#include "haskell/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      haskell::HaskellSharedServices, haskell::register_lsp_services>(argc, argv);
}
