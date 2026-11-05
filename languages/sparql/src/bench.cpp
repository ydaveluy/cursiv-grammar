#include "cursiv/LanguageBench.hpp"
#include "sparql/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sparql::SparqlSharedServices, sparql::register_lsp_services>(argc, argv);
}
