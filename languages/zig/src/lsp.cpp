// The zig language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "zig/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      zig::ZigSharedServices>(
      argc, argv, "zig-lsp", &zig::register_lsp_services);
}
