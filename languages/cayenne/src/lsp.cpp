// The cayenne language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "cayenne/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      cayenne::CayenneSharedServices>(
      argc, argv, "cayenne-lsp", &cayenne::register_lsp_services);
}
