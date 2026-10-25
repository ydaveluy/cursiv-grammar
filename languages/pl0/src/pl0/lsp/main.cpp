// The pl0 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "pl0/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      pl0::Pl0SharedServices>(
      argc, argv, "pl0-lsp", &pl0::register_lsp_services);
}
