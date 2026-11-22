// The miniscript language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "miniscript/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      miniscript::MiniscriptSharedServices>(
      argc, argv, "miniscript-lsp", &miniscript::register_lsp_services);
}
