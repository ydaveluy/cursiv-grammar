// The sharc language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "sharc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      sharc::SharcSharedServices>(
      argc, argv, "sharc-lsp", &sharc::register_lsp_services);
}
