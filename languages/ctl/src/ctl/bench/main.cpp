#include "cursiv/bench/LanguageBench.hpp"
#include "ctl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      ctl::CtlSharedServices, ctl::register_lsp_services>(argc, argv);
}
