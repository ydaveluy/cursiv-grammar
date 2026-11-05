#include "cursiv/LanguageBench.hpp"
#include "cql3/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cql3::Cql3SharedServices, cql3::register_lsp_services>(argc, argv);
}
