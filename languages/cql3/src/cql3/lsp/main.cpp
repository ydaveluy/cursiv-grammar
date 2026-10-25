// The cql3 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "cql3/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      cql3::Cql3SharedServices>(
      argc, argv, "cql3-lsp", &cql3::register_lsp_services);
}
