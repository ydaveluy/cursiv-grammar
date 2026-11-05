#include "cursiv/LanguageBench.hpp"
#include "cql/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cql::CqlSharedServices, cql::register_lsp_services>(argc, argv);
}
