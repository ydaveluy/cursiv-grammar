// The cobol85 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "cobol85/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      cobol85::Cobol85SharedServices>(
      argc, argv, "cobol85-lsp", &cobol85::register_lsp_services);
}
