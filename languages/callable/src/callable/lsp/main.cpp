// The callable language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "callable/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      callable::CallableSharedServices>(
      argc, argv, "callable-lsp", &callable::register_lsp_services);
}
