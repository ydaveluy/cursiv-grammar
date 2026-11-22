// The chip8 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "chip8/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      chip8::Chip8SharedServices>(
      argc, argv, "chip8-lsp", &chip8::register_lsp_services);
}
