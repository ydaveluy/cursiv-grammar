#include "cursiv/bench/LanguageBench.hpp"
#include "alef/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      alef::AlefSharedServices, alef::register_lsp_services>(argc, argv);
}
