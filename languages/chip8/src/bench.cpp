#include "cursiv/LanguageBench.hpp"
#include "chip8/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      chip8::Chip8SharedServices, chip8::register_lsp_services>(argc, argv);
}
