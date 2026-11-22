// The plucid language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "plucid/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      plucid::PlucidSharedServices>(
      argc, argv, "plucid-lsp", &plucid::register_lsp_services);
}
