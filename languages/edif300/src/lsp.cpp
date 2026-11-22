// The edif300 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "edif300/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      edif300::Edif300SharedServices>(
      argc, argv, "edif300-lsp", &edif300::register_lsp_services);
}
