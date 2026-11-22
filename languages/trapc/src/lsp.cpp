// The trapc language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "trapc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      trapc::TrapcSharedServices>(
      argc, argv, "trapc-lsp", &trapc::register_lsp_services);
}
