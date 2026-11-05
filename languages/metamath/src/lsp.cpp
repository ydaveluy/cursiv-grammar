// The metamath language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "metamath/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      metamath::MetamathSharedServices>(
      argc, argv, "metamath-lsp", &metamath::register_lsp_services);
}
