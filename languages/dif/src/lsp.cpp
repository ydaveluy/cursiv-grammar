// The dif language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "dif/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      dif::DifSharedServices>(
      argc, argv, "dif-lsp", &dif::register_lsp_services);
}
