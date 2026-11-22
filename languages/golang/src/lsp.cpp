// The golang language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "golang/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      golang::GolangSharedServices>(
      argc, argv, "golang-lsp", &golang::register_lsp_services);
}
