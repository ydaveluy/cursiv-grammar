#include "cursiv/bench/LanguageBench.hpp"
#include "scala/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      scala::ScalaSharedServices, scala::register_lsp_services>(argc, argv);
}
