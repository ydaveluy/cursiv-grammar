#include "cursiv/bench/LanguageBench.hpp"
#include "yara/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      yara::YaraSharedServices, yara::register_lsp_services>(argc, argv);
}
