#include "cursiv/bench/LanguageBench.hpp"
#include "graphql/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      graphql::GraphqlSharedServices, graphql::register_lsp_services>(argc, argv);
}
