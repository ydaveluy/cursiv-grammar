#include "cursiv/bench/LanguageBench.hpp"
#include "plucid/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      plucid::PlucidSharedServices, plucid::register_lsp_services>(argc, argv);
}
