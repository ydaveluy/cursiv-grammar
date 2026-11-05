#include "cursiv/LanguageBench.hpp"
#include "gql/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      gql::GqlSharedServices, gql::register_lsp_services>(argc, argv);
}
