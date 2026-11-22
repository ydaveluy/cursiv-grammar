#include "cursiv/LanguageBench.hpp"
#include "iri/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      iri::IriSharedServices, iri::register_lsp_services>(argc, argv);
}
