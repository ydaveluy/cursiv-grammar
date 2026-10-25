#include "cursiv/bench/LanguageBench.hpp"
#include "gql/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      gql::GqlSharedServices, gql::register_lsp_services>(argc, argv);
}
