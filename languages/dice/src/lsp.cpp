// The dice language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "dice/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      dice::DiceSharedServices>(
      argc, argv, "dice-lsp", &dice::register_lsp_services);
}
