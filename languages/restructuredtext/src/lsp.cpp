// The restructuredtext language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "restructuredtext/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      restructuredtext::RestructuredtextSharedServices>(
      argc, argv, "restructuredtext-lsp", &restructuredtext::register_lsp_services);
}
