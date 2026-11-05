#include "cursiv/LanguageBench.hpp"
#include "cypher/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cypher::CypherSharedServices, cypher::register_lsp_services>(argc, argv);
}
