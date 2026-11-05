#include "cursiv/LanguageBench.hpp"
#include "dice/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      dice::DiceSharedServices, dice::register_lsp_services>(argc, argv);
}
