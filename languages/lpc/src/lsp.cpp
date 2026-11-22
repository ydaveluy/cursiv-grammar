// The lpc language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "lpc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      lpc::LpcSharedServices>(
      argc, argv, "lpc-lsp", &lpc::register_lsp_services);
}
