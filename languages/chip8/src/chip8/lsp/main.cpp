// The chip8 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "chip8/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      chip8::Chip8SharedServices>(
      argc, argv, "chip8-lsp", &chip8::register_lsp_services);
}
