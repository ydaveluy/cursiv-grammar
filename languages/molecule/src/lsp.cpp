// The molecule language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "molecule/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      molecule::MoleculeSharedServices>(
      argc, argv, "molecule-lsp", &molecule::register_lsp_services);
}
