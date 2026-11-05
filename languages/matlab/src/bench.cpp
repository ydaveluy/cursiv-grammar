#include "cursiv/LanguageBench.hpp"
#include "matlab/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      matlab::MatlabSharedServices, matlab::register_lsp_services>(argc, argv);
}
