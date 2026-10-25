#include "cursiv/bench/LanguageBench.hpp"
#include "calculator/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      calculator::CalculatorSharedServices, calculator::register_lsp_services>(argc, argv);
}
