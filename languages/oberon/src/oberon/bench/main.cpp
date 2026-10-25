#include "cursiv/bench/LanguageBench.hpp"
#include "oberon/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      oberon::OberonSharedServices, oberon::register_lsp_services>(argc, argv);
}
