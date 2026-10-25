#include "cursiv/bench/LanguageBench.hpp"
#include "velocity/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      velocity::VelocitySharedServices, velocity::register_lsp_services>(argc, argv);
}
