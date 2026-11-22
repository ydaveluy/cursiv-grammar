// The bicep language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "bicep/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      bicep::BicepSharedServices>(
      argc, argv, "bicep-lsp", &bicep::register_lsp_services);
}
