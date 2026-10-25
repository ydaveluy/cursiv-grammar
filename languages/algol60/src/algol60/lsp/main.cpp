// The algol60 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "algol60/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      algol60::Algol60SharedServices>(
      argc, argv, "algol60-lsp", &algol60::register_lsp_services);
}
