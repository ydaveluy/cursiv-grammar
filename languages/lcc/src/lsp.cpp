// The lcc language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "lcc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      lcc::LccSharedServices>(
      argc, argv, "lcc-lsp", &lcc::register_lsp_services);
}
