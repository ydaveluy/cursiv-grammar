#include "cursiv/bench/LanguageBench.hpp"
#include "wln/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      wln::WlnSharedServices, wln::register_lsp_services>(argc, argv);
}
