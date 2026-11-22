// The javadoc language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "javadoc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      javadoc::JavadocSharedServices>(
      argc, argv, "javadoc-lsp", &javadoc::register_lsp_services);
}
