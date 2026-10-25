#include "cursiv/bench/LanguageBench.hpp"
#include "json5/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      json5::Json5SharedServices, json5::register_lsp_services>(argc, argv);
}
