// The agc language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "agc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      agc::AgcSharedServices>(
      argc, argv, "agc-lsp", &agc::register_lsp_services);
}
