// The racketisl language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "racketisl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      racketisl::RacketIslSharedServices>(
      argc, argv, "racketisl-lsp", &racketisl::register_lsp_services);
}
