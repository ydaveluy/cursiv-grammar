// The snobol language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "snobol/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      snobol::SnobolSharedServices>(
      argc, argv, "snobol-lsp", &snobol::register_lsp_services);
}
