#include "cursiv/LanguageBench.hpp"
#include "muparser/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      muparser::MuparserSharedServices, muparser::register_lsp_services>(argc, argv);
}
