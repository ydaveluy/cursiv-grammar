// The apt language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "apt/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      apt::AptSharedServices>(
      argc, argv, "apt-lsp", &apt::register_lsp_services);
}
