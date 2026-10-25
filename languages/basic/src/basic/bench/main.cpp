#include "cursiv/bench/LanguageBench.hpp"
#include "basic/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      basic::BasicSharedServices, basic::register_lsp_services>(argc, argv);
}
