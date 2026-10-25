#include "cursiv/bench/LanguageBench.hpp"
#include "tinyc/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      tinyc::TinycSharedServices, tinyc::register_lsp_services>(argc, argv);
}
