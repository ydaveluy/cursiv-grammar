#include "cursiv/bench/LanguageBench.hpp"
#include "plantuml/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      plantuml::PlantUMLSharedServices, plantuml::register_lsp_services>(argc, argv);
}
