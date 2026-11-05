// The tnt language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "tnt/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      tnt::TntSharedServices>(
      argc, argv, "tnt-lsp", &tnt::register_lsp_services);
}
