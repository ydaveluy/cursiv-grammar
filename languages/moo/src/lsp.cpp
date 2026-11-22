// The moo language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "moo/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      moo::MooSharedServices>(
      argc, argv, "moo-lsp", &moo::register_lsp_services);
}
