#include "cursiv/bench/LanguageBench.hpp"
#include "http/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      http::HttpSharedServices, http::register_lsp_services>(argc, argv);
}
