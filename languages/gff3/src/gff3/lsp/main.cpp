// The gff3 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "gff3/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      gff3::Gff3SharedServices>(
      argc, argv, "gff3-lsp", &gff3::register_lsp_services);
}
