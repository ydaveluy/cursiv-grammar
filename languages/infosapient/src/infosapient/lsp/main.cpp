// The infosapient language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "infosapient/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      infosapient::InfosapientSharedServices>(
      argc, argv, "infosapient-lsp", &infosapient::register_lsp_services);
}
