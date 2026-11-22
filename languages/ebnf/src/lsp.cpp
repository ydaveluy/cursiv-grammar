// The ebnf language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "ebnf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      ebnf::EbnfSharedServices>(
      argc, argv, "ebnf-lsp", &ebnf::register_lsp_services);
}
