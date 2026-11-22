// The sici language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "sici/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      sici::SiciSharedServices>(
      argc, argv, "sici-lsp", &sici::register_lsp_services);
}
