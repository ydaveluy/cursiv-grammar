#include "cursiv/bench/LanguageBench.hpp"
#include "rego/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      rego::RegoSharedServices, rego::register_lsp_services>(argc, argv);
}
