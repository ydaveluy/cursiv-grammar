// The prolog language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "prolog/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      prolog::PrologSharedServices>(
      argc, argv, "prolog-lsp", &prolog::register_lsp_services);
}
