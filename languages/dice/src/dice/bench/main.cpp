#include "cursiv/bench/LanguageBench.hpp"
#include "dice/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      dice::DiceSharedServices, dice::register_lsp_services>(argc, argv);
}
