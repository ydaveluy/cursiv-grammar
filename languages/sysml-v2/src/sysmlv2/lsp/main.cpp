// The sysmlv2 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "sysmlv2/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      sysmlv2::SysmlV2SharedServices>(
      argc, argv, "sysmlv2-lsp", &sysmlv2::register_lsp_services);
}
