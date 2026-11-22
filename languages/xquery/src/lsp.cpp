// The xquery language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "xquery/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      xquery::XQuerySharedServices>(
      argc, argv, "xquery-lsp", &xquery::register_lsp_services);
}
