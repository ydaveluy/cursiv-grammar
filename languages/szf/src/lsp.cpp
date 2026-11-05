// The szf language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "szf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      szf::SzfSharedServices>(
      argc, argv, "szf-lsp", &szf::register_lsp_services);
}
