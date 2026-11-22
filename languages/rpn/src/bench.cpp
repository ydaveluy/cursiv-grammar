#include "cursiv/LanguageBench.hpp"
#include "rpn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      rpn::RpnSharedServices, rpn::register_lsp_services>(argc, argv);
}
