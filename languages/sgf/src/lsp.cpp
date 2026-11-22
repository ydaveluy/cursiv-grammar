// The sgf language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "sgf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      sgf::SgfSharedServices>(
      argc, argv, "sgf-lsp", &sgf::register_lsp_services);
}
