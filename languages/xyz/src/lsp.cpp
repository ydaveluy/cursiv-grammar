// The xyz language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "xyz/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      xyz::XyzSharedServices>(
      argc, argv, "xyz-lsp", &xyz::register_lsp_services);
}
