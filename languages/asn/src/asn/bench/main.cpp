#include "cursiv/bench/LanguageBench.hpp"
#include "asn/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      asn::AsnSharedServices, asn::register_lsp_services>(argc, argv);
}
