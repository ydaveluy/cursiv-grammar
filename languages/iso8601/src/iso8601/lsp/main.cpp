// The iso8601 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "iso8601/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      iso8601::Iso8601SharedServices>(
      argc, argv, "iso8601-lsp", &iso8601::register_lsp_services);
}
