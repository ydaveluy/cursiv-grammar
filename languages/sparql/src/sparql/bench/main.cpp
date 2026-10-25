#include "cursiv/bench/LanguageBench.hpp"
#include "sparql/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sparql::SparqlSharedServices, sparql::register_lsp_services>(argc, argv);
}
