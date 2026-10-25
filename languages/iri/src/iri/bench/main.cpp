#include "cursiv/bench/LanguageBench.hpp"
#include "iri/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      iri::IriSharedServices, iri::register_lsp_services>(argc, argv);
}
