// The abb language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "abb/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      abb::AbbSharedServices>(
      argc, argv, "abb-lsp", &abb::register_lsp_services);
}
