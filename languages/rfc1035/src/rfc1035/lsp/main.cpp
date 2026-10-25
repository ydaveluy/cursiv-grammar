// The rfc1035 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "rfc1035/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      rfc1035::Rfc1035SharedServices>(
      argc, argv, "rfc1035-lsp", &rfc1035::register_lsp_services);
}
