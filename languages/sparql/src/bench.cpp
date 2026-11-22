#include "cursiv/LanguageBench.hpp"
#include "sparql/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      sparql::SparqlSharedServices, sparql::register_lsp_services>(argc, argv);
}
