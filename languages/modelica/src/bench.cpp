#include "cursiv/LanguageBench.hpp"
#include "modelica/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      modelica::ModelicaSharedServices, modelica::register_lsp_services>(argc, argv);
}
