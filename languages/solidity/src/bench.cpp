#include "cursiv/LanguageBench.hpp"
#include "solidity/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      solidity::SoliditySharedServices, solidity::register_lsp_services>(argc, argv);
}
