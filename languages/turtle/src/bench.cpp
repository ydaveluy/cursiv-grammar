#include "cursiv/LanguageBench.hpp"
#include "turtle/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      turtle::TurtleSharedServices, turtle::register_lsp_services>(argc, argv);
}
