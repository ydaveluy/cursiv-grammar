// The p language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "p/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      p::PSharedServices>(
      argc, argv, "p-lsp", &p::register_lsp_services);
}
