// The xquery language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "xquery/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      xquery::XQuerySharedServices>(
      argc, argv, "xquery-lsp", &xquery::register_lsp_services);
}
