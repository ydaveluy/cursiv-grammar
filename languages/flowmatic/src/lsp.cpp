// The flowmatic language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "flowmatic/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      flowmatic::FlowmaticSharedServices>(
      argc, argv, "flowmatic-lsp", &flowmatic::register_lsp_services);
}
