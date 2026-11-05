// The powerbuilder language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "powerbuilder/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      powerbuilder::PowerbuilderSharedServices>(
      argc, argv, "powerbuilder-lsp", &powerbuilder::register_lsp_services);
}
