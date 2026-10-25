// The edif300 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "edif300/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      edif300::Edif300SharedServices>(
      argc, argv, "edif300-lsp", &edif300::register_lsp_services);
}
