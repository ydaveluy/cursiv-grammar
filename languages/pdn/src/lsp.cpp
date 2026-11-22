// The pdn language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "pdn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      pdn::PdnSharedServices>(
      argc, argv, "pdn-lsp", &pdn::register_lsp_services);
}
