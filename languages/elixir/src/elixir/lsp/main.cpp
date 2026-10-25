// The elixir language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "elixir/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      elixir::ElixirSharedServices>(
      argc, argv, "elixir-lsp", &elixir::register_lsp_services);
}
