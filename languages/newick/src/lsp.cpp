// The newick language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "newick/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      newick::NewickSharedServices>(
      argc, argv, "newick-lsp", &newick::register_lsp_services);
}
