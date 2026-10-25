#include "cursiv/bench/LanguageBench.hpp"
#include "stl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      stl::StlSharedServices, stl::register_lsp_services>(argc, argv);
}
