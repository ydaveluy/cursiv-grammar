// The cto language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "cto/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      cto::CtoSharedServices>(
      argc, argv, "cto-lsp", &cto::register_lsp_services);
}
