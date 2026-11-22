// The kirikiritjs language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "kirikiritjs/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      kirikiritjs::KirikiriTjsSharedServices>(
      argc, argv, "kirikiritjs-lsp", &kirikiritjs::register_lsp_services);
}
