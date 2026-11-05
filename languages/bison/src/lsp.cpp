// The bison language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "bison/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      bison::BisonSharedServices>(
      argc, argv, "bison-lsp", &bison::register_lsp_services);
}
