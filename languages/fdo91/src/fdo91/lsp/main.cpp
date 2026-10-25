// The fdo91 language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "fdo91/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      fdo91::Fdo91SharedServices>(
      argc, argv, "fdo91-lsp", &fdo91::register_lsp_services);
}
