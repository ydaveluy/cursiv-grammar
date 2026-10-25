// The rfc3080 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "rfc3080/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      rfc3080::Rfc3080SharedServices>(
      argc, argv, "rfc3080-lsp", &rfc3080::register_lsp_services);
}
