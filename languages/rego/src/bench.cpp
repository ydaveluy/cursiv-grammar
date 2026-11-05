#include "cursiv/LanguageBench.hpp"
#include "rego/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      rego::RegoSharedServices, rego::register_lsp_services>(argc, argv);
}
