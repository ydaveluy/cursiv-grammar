// The lrc language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "lrc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      lrc::LrcSharedServices>(
      argc, argv, "lrc-lsp", &lrc::register_lsp_services);
}
