// The r language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "r/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      r::RSharedServices>(
      argc, argv, "r-lsp", &r::register_lsp_services);
}
