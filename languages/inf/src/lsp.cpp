// The inf language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "inf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      inf::InfSharedServices>(
      argc, argv, "inf-lsp", &inf::register_lsp_services);
}
