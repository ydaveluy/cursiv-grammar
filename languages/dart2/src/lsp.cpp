// The dart2 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "dart2/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      dart2::Dart2SharedServices>(
      argc, argv, "dart2-lsp", &dart2::register_lsp_services);
}
