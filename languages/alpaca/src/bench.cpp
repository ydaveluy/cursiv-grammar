#include "cursiv/LanguageBench.hpp"
#include "alpaca/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      alpaca::AlpacaSharedServices, alpaca::register_lsp_services>(argc, argv);
}
