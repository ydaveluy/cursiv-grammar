// The vhdl language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "vhdl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      vhdl::VhdlSharedServices>(
      argc, argv, "vhdl-lsp", &vhdl::register_lsp_services);
}
