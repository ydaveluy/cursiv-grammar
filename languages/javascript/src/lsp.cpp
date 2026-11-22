// The javascript language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "javascript/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      javascript::JavaScriptSharedServices>(
      argc, argv, "javascript-lsp", &javascript::register_lsp_services);
}
