// The vba language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "vba/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      vba::VbaSharedServices>(
      argc, argv, "vba-lsp", &vba::register_lsp_services);
}
