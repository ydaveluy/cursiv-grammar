// The provn language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "provn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      provn::ProvNSharedServices>(
      argc, argv, "provn-lsp", &provn::register_lsp_services);
}
