#include "cursiv/LanguageBench.hpp"
#include "turtledoc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      turtledoc::TurtleDocSharedServices, turtledoc::register_lsp_services>(argc, argv);
}
