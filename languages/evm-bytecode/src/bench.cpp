#include "cursiv/LanguageBench.hpp"
#include "evmbytecode/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      evmbytecode::EvmBytecodeSharedServices, evmbytecode::register_lsp_services>(argc, argv);
}
