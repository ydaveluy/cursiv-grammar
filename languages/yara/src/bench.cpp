#include "cursiv/LanguageBench.hpp"
#include "yara/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      yara::YaraSharedServices, yara::register_lsp_services>(argc, argv);
}
