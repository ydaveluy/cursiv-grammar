#include "cursiv/LanguageBench.hpp"
#include "dice/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      dice::DiceSharedServices, dice::register_lsp_services>(argc, argv);
}
