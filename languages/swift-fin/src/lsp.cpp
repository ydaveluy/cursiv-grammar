// The swiftfin language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "swiftfin/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      swiftfin::SwiftFinSharedServices>(
      argc, argv, "swiftfin-lsp", &swiftfin::register_lsp_services);
}
