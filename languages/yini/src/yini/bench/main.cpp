#include "cursiv/bench/LanguageBench.hpp"
#include "yini/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      yini::YiniSharedServices, yini::register_lsp_services>(argc, argv);
}
