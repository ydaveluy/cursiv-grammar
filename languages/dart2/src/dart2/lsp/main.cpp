// The dart2 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "dart2/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      dart2::Dart2SharedServices>(
      argc, argv, "dart2-lsp", &dart2::register_lsp_services);
}
