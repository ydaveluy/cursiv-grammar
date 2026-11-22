// The janus language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "janus/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      janus::JanusSharedServices>(
      argc, argv, "janus-lsp", &janus::register_lsp_services);
}
