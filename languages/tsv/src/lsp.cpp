// The tsv language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "tsv/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      tsv::TsvSharedServices>(
      argc, argv, "tsv-lsp", &tsv::register_lsp_services);
}
