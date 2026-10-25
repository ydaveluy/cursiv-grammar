#include "cursiv/bench/LanguageBench.hpp"
#include "solidity/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      solidity::SoliditySharedServices, solidity::register_lsp_services>(argc, argv);
}
