#include "cursiv/bench/LanguageBench.hpp"
#include "cypher/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cypher::CypherSharedServices, cypher::register_lsp_services>(argc, argv);
}
