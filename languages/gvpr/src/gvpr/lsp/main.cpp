// The gvpr language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "gvpr/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      gvpr::GvprSharedServices>(
      argc, argv, "gvpr-lsp", &gvpr::register_lsp_services);
}
