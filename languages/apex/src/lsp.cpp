// The apex language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "apex/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      apex::ApexSharedServices>(
      argc, argv, "apex-lsp", &apex::register_lsp_services);
}
