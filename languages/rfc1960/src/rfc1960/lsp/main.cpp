// The rfc1960 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "rfc1960/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      rfc1960::Rfc1960SharedServices>(
      argc, argv, "rfc1960-lsp", &rfc1960::register_lsp_services);
}
