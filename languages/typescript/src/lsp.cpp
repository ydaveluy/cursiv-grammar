// The typescript language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "typescript/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      typescript::TypeScriptSharedServices>(
      argc, argv, "typescript-lsp", &typescript::register_lsp_services);
}
