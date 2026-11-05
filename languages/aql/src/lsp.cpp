// The aql language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "aql/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      aql::AqlSharedServices>(
      argc, argv, "aql-lsp", &aql::register_lsp_services);
}
