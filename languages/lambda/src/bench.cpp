#include "cursiv/LanguageBench.hpp"
#include "lambda/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lambda::LambdaSharedServices, lambda::register_lsp_services>(argc, argv);
}
