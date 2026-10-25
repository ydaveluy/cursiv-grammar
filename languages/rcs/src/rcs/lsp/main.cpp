// The rcs language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "rcs/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      rcs::RcsSharedServices>(
      argc, argv, "rcs-lsp", &rcs::register_lsp_services);
}
