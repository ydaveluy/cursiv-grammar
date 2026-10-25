#include "cursiv/bench/LanguageBench.hpp"
#include "modelica/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      modelica::ModelicaSharedServices, modelica::register_lsp_services>(argc, argv);
}
