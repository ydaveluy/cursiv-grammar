// The alef language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "alef/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      alef::AlefSharedServices>(
      argc, argv, "alef-lsp", &alef::register_lsp_services);
}
