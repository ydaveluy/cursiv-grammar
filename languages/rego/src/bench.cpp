#include "cursiv/LanguageBench.hpp"
#include "rego/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      rego::RegoSharedServices, rego::register_lsp_services>(argc, argv);
}
