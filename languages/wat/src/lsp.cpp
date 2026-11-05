// The wat language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "wat/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      wat::WatSharedServices>(
      argc, argv, "wat-lsp", &wat::register_lsp_services);
}
