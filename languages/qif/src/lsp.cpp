// The qif language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "qif/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      qif::QifSharedServices>(
      argc, argv, "qif-lsp", &qif::register_lsp_services);
}
