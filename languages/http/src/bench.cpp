#include "cursiv/LanguageBench.hpp"
#include "http/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      http::HttpSharedServices, http::register_lsp_services>(argc, argv);
}
