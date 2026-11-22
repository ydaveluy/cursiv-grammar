// The pii language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "pii/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      pii::PiiSharedServices>(
      argc, argv, "pii-lsp", &pii::register_lsp_services);
}
