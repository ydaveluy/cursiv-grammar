// The corundum language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "corundum/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      corundum::CorundumSharedServices>(
      argc, argv, "corundum-lsp", &corundum::register_lsp_services);
}
