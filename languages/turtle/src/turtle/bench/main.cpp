#include "cursiv/bench/LanguageBench.hpp"
#include "turtle/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      turtle::TurtleSharedServices, turtle::register_lsp_services>(argc, argv);
}
