// The bnf language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "bnf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      bnf::BnfSharedServices>(
      argc, argv, "bnf-lsp", &bnf::register_lsp_services);
}
