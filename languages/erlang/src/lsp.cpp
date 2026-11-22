// The erlang language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "erlang/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      erlang::ErlangSharedServices>(
      argc, argv, "erlang-lsp", &erlang::register_lsp_services);
}
