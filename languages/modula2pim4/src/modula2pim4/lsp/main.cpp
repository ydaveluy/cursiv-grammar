// The modula2pim4 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "modula2pim4/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      modula2pim4::Modula2pim4SharedServices>(
      argc, argv, "modula2pim4-lsp", &modula2pim4::register_lsp_services);
}
