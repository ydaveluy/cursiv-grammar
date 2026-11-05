#include "cursiv/LanguageBench.hpp"
#include "json/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      json::JsonSharedServices, json::register_lsp_services>(argc, argv);
}
