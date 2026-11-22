// The pike language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "pike/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      pike::PikeSharedServices>(
      argc, argv, "pike-lsp", &pike::register_lsp_services);
}
