// The wren language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "wren/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      wren::WrenSharedServices>(
      argc, argv, "wren-lsp", &wren::register_lsp_services);
}
