// The turtle language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "turtle/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      turtle::TurtleSharedServices>(
      argc, argv, "turtle-lsp", &turtle::register_lsp_services);
}
