// The stellaris language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "stellaris/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      stellaris::StellarisSharedServices>(
      argc, argv, "stellaris-lsp", &stellaris::register_lsp_services);
}
