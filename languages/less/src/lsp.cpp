// The less language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "less/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      less::LessSharedServices>(
      argc, argv, "less-lsp", &less::register_lsp_services);
}
