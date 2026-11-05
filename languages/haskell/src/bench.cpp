#include "cursiv/LanguageBench.hpp"
#include "haskell/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      haskell::HaskellSharedServices, haskell::register_lsp_services>(argc, argv);
}
