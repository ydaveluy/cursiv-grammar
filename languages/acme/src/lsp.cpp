// The acme language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "acme/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      acme::AcmeSharedServices>(
      argc, argv, "acme-lsp", &acme::register_lsp_services);
}
