// The alpaca language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "alpaca/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      alpaca::AlpacaSharedServices>(
      argc, argv, "alpaca-lsp", &alpaca::register_lsp_services);
}
