#include "cursiv/bench/LanguageBench.hpp"
#include "robotwars/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      robotwars::RobotwarsSharedServices, robotwars::register_lsp_services>(argc, argv);
}
