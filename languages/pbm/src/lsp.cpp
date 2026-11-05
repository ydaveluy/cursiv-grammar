// The pbm language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "pbm/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      pbm::PbmSharedServices>(
      argc, argv, "pbm-lsp", &pbm::register_lsp_services);
}
