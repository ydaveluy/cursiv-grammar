#include "cursiv/bench/LanguageBench.hpp"
#include "lolcode/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lolcode::LolcodeSharedServices, lolcode::register_lsp_services>(argc, argv);
}
