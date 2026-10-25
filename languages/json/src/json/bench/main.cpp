#include "cursiv/bench/LanguageBench.hpp"
#include "json/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      json::JsonSharedServices, json::register_lsp_services>(argc, argv);
}
