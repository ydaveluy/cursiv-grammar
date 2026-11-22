// The dcm language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "dcm/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      dcm::DcmSharedServices>(
      argc, argv, "dcm-lsp", &dcm::register_lsp_services);
}
