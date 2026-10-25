#include "cursiv/bench/LanguageBench.hpp"
#include "gml/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      gml::GmlSharedServices, gml::register_lsp_services>(argc, argv);
}
