// The verilog language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "verilog/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      verilog::VerilogSharedServices>(
      argc, argv, "verilog-lsp", &verilog::register_lsp_services);
}
