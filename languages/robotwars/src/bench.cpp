#include "cursiv/LanguageBench.hpp"
#include "robotwars/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      robotwars::RobotwarsSharedServices, robotwars::register_lsp_services>(argc, argv);
}
