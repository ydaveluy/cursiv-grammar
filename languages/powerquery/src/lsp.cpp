// The powerquery language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "powerquery/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      powerquery::PowerquerySharedServices>(
      argc, argv, "powerquery-lsp", &powerquery::register_lsp_services);
}
