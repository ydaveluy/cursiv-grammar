// The promql language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "promql/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      promql::PromqlSharedServices>(
      argc, argv, "promql-lsp", &promql::register_lsp_services);
}
