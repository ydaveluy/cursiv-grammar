#include "cursiv/LanguageBench.hpp"
#include "graphstreamdgs/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      graphstreamdgs::GraphstreamDgsSharedServices, graphstreamdgs::register_lsp_services>(argc, argv);
}
