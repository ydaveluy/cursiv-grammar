// The trac language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "trac/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      trac::TracSharedServices>(
      argc, argv, "trac-lsp", &trac::register_lsp_services);
}
