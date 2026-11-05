#include "cursiv/LanguageBench.hpp"
#include "turtledoc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      turtledoc::TurtleDocSharedServices, turtledoc::register_lsp_services>(argc, argv);
}
