#include "cursiv/bench/LanguageBench.hpp"
#include "xsmp/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      xsmp::XsmpSharedServices, xsmp::register_lsp_services>(argc, argv);
}
