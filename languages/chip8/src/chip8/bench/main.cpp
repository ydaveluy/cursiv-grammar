#include "cursiv/bench/LanguageBench.hpp"
#include "chip8/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      chip8::Chip8SharedServices, chip8::register_lsp_services>(argc, argv);
}
