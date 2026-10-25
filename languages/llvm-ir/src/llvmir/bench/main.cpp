#include "cursiv/bench/LanguageBench.hpp"
#include "llvmir/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      llvmir::LlvmIrSharedServices, llvmir::register_lsp_services>(argc, argv);
}
