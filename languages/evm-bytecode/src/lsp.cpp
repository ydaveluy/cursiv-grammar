// The evmbytecode language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "evmbytecode/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      evmbytecode::EvmBytecodeSharedServices>(
      argc, argv, "evmbytecode-lsp", &evmbytecode::register_lsp_services);
}
