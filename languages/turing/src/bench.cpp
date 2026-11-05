#include "cursiv/LanguageBench.hpp"
#include "turing/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      turing::TuringSharedServices, turing::register_lsp_services>(argc, argv);
}
