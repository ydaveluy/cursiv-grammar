#include "cursiv/LanguageBench.hpp"
#include "promql/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      promql::PromqlSharedServices, promql::register_lsp_services>(argc, argv);
}
