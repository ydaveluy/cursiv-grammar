#include "cursiv/LanguageBench.hpp"
#include "arithmetic/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      arithmetic::ArithmeticSharedServices, arithmetic::register_lsp_services>(argc, argv);
}
