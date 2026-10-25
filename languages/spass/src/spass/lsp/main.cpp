// The spass language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "spass/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      spass::SpassSharedServices>(
      argc, argv, "spass-lsp", &spass::register_lsp_services);
}
