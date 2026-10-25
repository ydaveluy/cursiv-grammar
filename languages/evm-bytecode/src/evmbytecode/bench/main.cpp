#include "cursiv/bench/LanguageBench.hpp"
#include "evmbytecode/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      evmbytecode::EvmBytecodeSharedServices, evmbytecode::register_lsp_services>(argc, argv);
}
