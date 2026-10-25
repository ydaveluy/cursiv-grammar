#include "cursiv/bench/LanguageBench.hpp"
#include "clif/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      clif::ClifSharedServices, clif::register_lsp_services>(argc, argv);
}
