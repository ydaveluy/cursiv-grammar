// The rfc822 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "rfc822/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      rfc822::Rfc822SharedServices>(
      argc, argv, "rfc822-lsp", &rfc822::register_lsp_services);
}
