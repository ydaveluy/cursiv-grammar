#include "cursiv/bench/LanguageBench.hpp"
#include "rpn/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      rpn::RpnSharedServices, rpn::register_lsp_services>(argc, argv);
}
