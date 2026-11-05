#include "cursiv/LanguageBench.hpp"
#include "velocity/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      velocity::VelocitySharedServices, velocity::register_lsp_services>(argc, argv);
}
