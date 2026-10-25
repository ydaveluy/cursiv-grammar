#include "cursiv/bench/LanguageBench.hpp"
#include "refal/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      refal::RefalSharedServices, refal::register_lsp_services>(argc, argv);
}
