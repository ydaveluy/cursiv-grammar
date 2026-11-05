// The pgn language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "pgn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      pgn::PgnSharedServices>(
      argc, argv, "pgn-lsp", &pgn::register_lsp_services);
}
