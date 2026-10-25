// The abnf language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "abnf/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      abnf::AbnfSharedServices>(
      argc, argv, "abnf-lsp", &abnf::register_lsp_services);
}
