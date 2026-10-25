#include "cursiv/bench/LanguageBench.hpp"
#include "arithmetic/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      arithmetic::ArithmeticSharedServices, arithmetic::register_lsp_services>(argc, argv);
}
