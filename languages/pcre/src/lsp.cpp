// The pcre language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "pcre/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      pcre::PcreSharedServices>(
      argc, argv, "pcre-lsp", &pcre::register_lsp_services);
}
