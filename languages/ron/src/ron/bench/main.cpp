#include "cursiv/bench/LanguageBench.hpp"
#include "ron/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      ron::RonSharedServices, ron::register_lsp_services>(argc, argv);
}
