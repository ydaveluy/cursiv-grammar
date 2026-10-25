#include "cursiv/bench/LanguageBench.hpp"
#include "ocl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      ocl::OclSharedServices, ocl::register_lsp_services>(argc, argv);
}
