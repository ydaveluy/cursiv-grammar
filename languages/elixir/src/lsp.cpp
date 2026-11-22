// The elixir language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "elixir/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      elixir::ElixirSharedServices>(
      argc, argv, "elixir-lsp", &elixir::register_lsp_services);
}
