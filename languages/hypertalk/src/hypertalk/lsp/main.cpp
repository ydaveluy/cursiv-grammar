// The hypertalk language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "hypertalk/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      hypertalk::HypertalkSharedServices>(
      argc, argv, "hypertalk-lsp", &hypertalk::register_lsp_services);
}
