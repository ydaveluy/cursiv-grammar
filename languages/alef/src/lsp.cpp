// The alef language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "alef/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      alef::AlefSharedServices>(
      argc, argv, "alef-lsp", &alef::register_lsp_services);
}
