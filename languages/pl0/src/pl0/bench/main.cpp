#include "cursiv/bench/LanguageBench.hpp"
#include "pl0/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pl0::Pl0SharedServices, pl0::register_lsp_services>(argc, argv);
}
