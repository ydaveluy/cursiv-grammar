// The wktcrsv1 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "wktcrsv1/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      wktcrsv1::WktCrsV1SharedServices>(
      argc, argv, "wktcrsv1-lsp", &wktcrsv1::register_lsp_services);
}
