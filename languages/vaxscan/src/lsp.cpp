// The vaxscan language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "vaxscan/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      vaxscan::VaxscanSharedServices>(
      argc, argv, "vaxscan-lsp", &vaxscan::register_lsp_services);
}
