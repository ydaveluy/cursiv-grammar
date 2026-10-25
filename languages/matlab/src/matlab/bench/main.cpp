#include "cursiv/bench/LanguageBench.hpp"
#include "matlab/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      matlab::MatlabSharedServices, matlab::register_lsp_services>(argc, argv);
}
