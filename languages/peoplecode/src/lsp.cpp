// The peoplecode language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "peoplecode/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      peoplecode::PeoplecodeSharedServices>(
      argc, argv, "peoplecode-lsp", &peoplecode::register_lsp_services);
}
