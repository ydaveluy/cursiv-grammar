// The redcode language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "redcode/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      redcode::RedcodeSharedServices>(
      argc, argv, "redcode-lsp", &redcode::register_lsp_services);
}
