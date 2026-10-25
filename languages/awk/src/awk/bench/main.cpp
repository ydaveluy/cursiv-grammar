#include "cursiv/bench/LanguageBench.hpp"
#include "awk/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      awk::AwkSharedServices, awk::register_lsp_services>(argc, argv);
}
