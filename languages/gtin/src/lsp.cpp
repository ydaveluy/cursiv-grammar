// The gtin language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "gtin/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      gtin::GtinSharedServices>(
      argc, argv, "gtin-lsp", &gtin::register_lsp_services);
}
