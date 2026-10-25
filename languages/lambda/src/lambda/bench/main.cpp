#include "cursiv/bench/LanguageBench.hpp"
#include "lambda/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lambda::LambdaSharedServices, lambda::register_lsp_services>(argc, argv);
}
