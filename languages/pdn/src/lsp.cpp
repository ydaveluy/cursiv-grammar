// The pdn language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "pdn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      pdn::PdnSharedServices>(
      argc, argv, "pdn-lsp", &pdn::register_lsp_services);
}
