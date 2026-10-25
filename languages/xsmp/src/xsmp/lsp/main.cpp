// The xsmp language server: one language, the standard transport.

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "xsmp/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<xsmp::XsmpSharedServices>(
      argc, argv, "xsmp-lsp", &xsmp::register_lsp_services);
}
