// The telephone language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "telephone/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      telephone::TelephoneSharedServices>(
      argc, argv, "telephone-lsp", &telephone::register_lsp_services);
}
