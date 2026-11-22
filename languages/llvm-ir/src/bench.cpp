#include "cursiv/LanguageBench.hpp"
#include "llvmir/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      llvmir::LlvmIrSharedServices, llvmir::register_lsp_services>(argc, argv);
}
