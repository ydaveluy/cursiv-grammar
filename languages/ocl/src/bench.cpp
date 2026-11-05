#include "cursiv/LanguageBench.hpp"
#include "ocl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      ocl::OclSharedServices, ocl::register_lsp_services>(argc, argv);
}
