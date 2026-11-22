#include "cursiv/LanguageBench.hpp"
#include "asn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      asn::AsnSharedServices, asn::register_lsp_services>(argc, argv);
}
