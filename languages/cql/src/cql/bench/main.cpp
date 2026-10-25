#include "cursiv/bench/LanguageBench.hpp"
#include "cql/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cql::CqlSharedServices, cql::register_lsp_services>(argc, argv);
}
