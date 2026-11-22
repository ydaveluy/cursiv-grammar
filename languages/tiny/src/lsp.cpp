// The tiny language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "tiny/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      tiny::TinySharedServices>(
      argc, argv, "tiny-lsp", &tiny::register_lsp_services);
}
