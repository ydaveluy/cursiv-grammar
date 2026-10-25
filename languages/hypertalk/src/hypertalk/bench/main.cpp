#include "cursiv/bench/LanguageBench.hpp"
#include "hypertalk/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      hypertalk::HypertalkSharedServices, hypertalk::register_lsp_services>(argc, argv);
}
