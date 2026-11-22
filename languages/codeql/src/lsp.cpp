// The codeql language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "codeql/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      codeql::CodeqlSharedServices>(
      argc, argv, "codeql-lsp", &codeql::register_lsp_services);
}
