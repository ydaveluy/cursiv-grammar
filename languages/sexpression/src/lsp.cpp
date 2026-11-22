// The sexpression language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "sexpression/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      sexpression::SexpressionSharedServices>(
      argc, argv, "sexpression-lsp", &sexpression::register_lsp_services);
}
