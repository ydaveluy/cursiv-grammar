// The lucene language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "lucene/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      lucene::LuceneSharedServices>(
      argc, argv, "lucene-lsp", &lucene::register_lsp_services);
}
