// The postalcode language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "postalcode/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      postalcode::PostalcodeSharedServices>(
      argc, argv, "postalcode-lsp", &postalcode::register_lsp_services);
}
