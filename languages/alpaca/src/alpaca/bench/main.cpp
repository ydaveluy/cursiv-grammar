#include "cursiv/bench/LanguageBench.hpp"
#include "alpaca/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      alpaca::AlpacaSharedServices, alpaca::register_lsp_services>(argc, argv);
}
